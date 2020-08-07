/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/thread.h>
#include <sys/sem.h>

#include <machine/cpuregs.h>
#include <machine/cpufunc.h>
#include <machine/frame.h>
#include <machine/cache_mipsNN.h>
#include <machine/cache_r4k.h>

#include <mips/beri/beri_epw.h>
#include <dev/altera/msgdma/msgdma.h>
#include <dev/altera/fifo/fifo.h>

#include "device-model.h"
#include "emul.h"
#include "emul_msgdma.h"
#include "emul_iommu.h"

#define	EMUL_MSGDMA_DEBUG
#undef	EMUL_MSGDMA_DEBUG

#ifdef	EMUL_MSGDMA_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

#define	SOFTDMA_RX_EVENTS	\
	(A_ONCHIP_FIFO_MEM_CORE_INTR_FULL	| \
	 A_ONCHIP_FIFO_MEM_CORE_INTR_OVERFLOW	| \
	 A_ONCHIP_FIFO_MEM_CORE_INTR_UNDERFLOW)

static struct msgdma_desc *
emul_msgdma_next_desc(struct msgdma_desc *desc0)
{
	struct msgdma_desc *desc;
	uint64_t addr;

	addr = le32toh(desc0->next);
	addr |= MIPS_XKPHYS_UNCACHED_BASE;
	desc = (struct msgdma_desc *)addr;

	return (desc);
}

static void
send_soft_irq(struct msgdma_softc *sc)
{
	struct msgdma_csr *csr;
	uint64_t addr;

	dprintf("%s\n", __func__);

	csr = &sc->csr;
	if ((csr->dma_control & CONTROL_GIEM) == 0)
		return;

	addr = BERIPIC0_IP_SET | MIPS_XKPHYS_UNCACHED_BASE;

	if (sc->unit == 1)
		*(volatile uint64_t *)(addr) = (1 << DM_MSGDMA1_INTR);
	else
		*(volatile uint64_t *)(addr) = (1 << DM_MSGDMA0_INTR);
}

void
emul_msgdma_poll(struct msgdma_softc *sc)
{
	struct msgdma_desc *desc;
	struct iovec iov;
	uint64_t base;
	uint32_t intr;
	uint32_t reg;
	uint32_t control;
	int processed;
	int count;

	if (sc->poll_en == 0)
		return;

	intr = intr_disable();

	count = 0;
	do {
		desc = sc->cur_desc;
		reg = le32toh(desc->control);
		if ((reg & CONTROL_OWN) == 0)
			break;

		if (sc->unit == 0)
			base = le32toh(desc->read_lo);
		else
			base = le32toh(desc->write_lo);

#if defined(ALTERA_MSGDMA_DESC_EXT) || defined(ALTERA_MSGDMA_DESC_PF_EXT)
		if (sc->unit == 0)
			base |= (uint64_t)le32toh(desc->read_hi) << 32;
		else
			base |= (uint64_t)le32toh(desc->write_hi) << 32;
#endif

#if 0
		if (sc->unit == 0) {
			printf("%s%d: base %lx (lo %x hi %x) size %lx\n",
			    __func__, sc->unit, base, desc->read_lo,
			    desc->read_hi, le32toh(desc->length));
		} else {
			printf("%s%d: base %lx (lo %x hi %x) size %lx\n",
			    __func__, sc->unit, base, desc->write_lo,
			    desc->write_hi, le32toh(desc->length));
		}
#endif

		iov.iov_len = le32toh(desc->length);
#ifdef CONFIG_IOMMU
		iov.iov_base = (void *) (base);
#else
		iov.iov_base = (void *) (base | MIPS_XKPHYS_UNCACHED_BASE);
#endif

		__asm __volatile("sync;sync;sync");

		emul_iommu_activate(sc->iommu_sc);
		if (sc->unit == 0)
			processed = fifo_process_tx(sc->fifo_sc, &iov, 1);
		else
			processed = fifo_process_rx(sc->fifo_sc, &iov, 1, 0);
		if (processed <= 0)
			break;

		desc->transferred = htole32(processed);
		control = le32toh(desc->control);
		control &= ~CONTROL_OWN;
		desc->control = htole32(control);
		__asm __volatile("sync;sync;sync");

		count++;
		sc->cur_desc = emul_msgdma_next_desc(desc);
	} while (sc->cur_desc);

	intr_restore(intr);

	if (count > 0)
		send_soft_irq(sc);
}

static int
emul_msgdma_poll_enable(struct msgdma_softc *sc)
{
	struct msgdma_pf *pf;
	uint64_t addr;

	pf = &sc->pf;
	if (pf->pf_next_lo == 0)
		return (-1);

	addr = pf->pf_next_lo | MIPS_XKPHYS_UNCACHED_BASE;

	sc->cur_desc = (struct msgdma_desc *)addr;
	sc->poll_en = 1;

	fifo_interrupts_disable(sc->fifo_sc);

	if (sc->unit == 1) {
		dprintf("%s(%d): Enabling RX interrupts\n",
		    __func__, sc->unit);
		sc->fifo_sc->cb = emul_msgdma_fifo_intr;
		sc->fifo_sc->cb_arg = sc;
		fifo_interrupts_enable(sc->fifo_sc, SOFTDMA_RX_EVENTS);
	}

	return (0);
}

static void
csr_r(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset)
{
	struct msgdma_csr *csr;

	csr = &sc->csr;

	switch (offset) {
	case DMA_STATUS:
		dprintf("%s: DMA_STATUS\n", __func__);
		bcopy((void *)&csr->dma_status, (void *)req->data, 4);
		break;
	case DMA_CONTROL:
		dprintf("%s: DMA_CONTROL\n", __func__);
		bcopy((void *)&csr->dma_control, (void *)req->data, 4);
		break;
	};
}

static void
csr_w(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset, uint64_t val)
{
	struct msgdma_csr *csr;

	csr = &sc->csr;

	switch (offset) {
	case DMA_STATUS:
		dprintf("%s: DMA_STATUS, val %x\n", __func__, val);
		csr->dma_status = val;
		break;
	case DMA_CONTROL:
		dprintf("%s: DMA_CONTROL, val %x\n", __func__, val);
		if (val & CONTROL_RESET) {
			csr->dma_status = 0;
			csr->dma_control = 0;
		} else {
			csr->dma_control = val;
		}
		break;
	};
}

static void
pf_r(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset)
{
	struct msgdma_pf *pf;

	pf = &sc->pf;

	switch (offset) {
	case PF_CONTROL:
		dprintf("%s: PF_CONTROL\n", __func__);
		bcopy(&pf->pf_control, req->data, 4);
		break;
	case PF_NEXT_LO:
		dprintf("%s: PF_NEXT_LO\n", __func__);
		bcopy((void *)&pf->pf_next_lo, (void *)req->data, 4);
		break;
	case PF_NEXT_HI:
		dprintf("%s: PF_NEXT_HI\n", __func__);
		bcopy(&pf->pf_next_hi, req->data, 4);
		break;
	case PF_POLL_FREQ:
		dprintf("%s: PF_POLL_FREQ\n", __func__);
		bcopy(&pf->pf_poll_freq, req->data, 4);
		break;
	case PF_STATUS:
		dprintf("%s: PF_STATUS\n", __func__);
		bcopy(&pf->pf_status, req->data, 4);
		break;
	};
}

static void
pf_w(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset, uint64_t val)
{
	struct msgdma_pf *pf;

	pf = &sc->pf;

	switch (offset) {
	case PF_CONTROL:
		dprintf("%s: PF_CONTROL val %lx\n", __func__, val);
		pf->pf_control = val;
		break;
	case PF_NEXT_LO:
		dprintf("%s: PF_NEXT_LO val %lx\n", __func__, val);
		pf->pf_next_lo = val;
		break;
	case PF_NEXT_HI:
		dprintf("%s: PF_NEXT_HI val %lx\n", __func__, val);
		pf->pf_next_hi = val;
		break;
	case PF_POLL_FREQ:
		dprintf("%s: PF_POLL_FREQ val %lx\n", __func__, val);
		pf->pf_poll_freq = val;
		if (val != 0) {
			printf("%s: enabling polling for msgdma unit%d\n",
			    __func__, sc->unit);
			emul_msgdma_poll_enable(sc);
		}
		break;
	case PF_STATUS:
		dprintf("%s: PF_STATUS val %lx\n", __func__, val);
		pf->pf_status = val;
		break;
	};
}

void
emul_msgdma(const struct emul_link *elink, struct epw_softc *epw_sc,
    struct epw_request *req)
{
	struct msgdma_softc *sc;
	uint64_t offset;
	uint64_t val;

	sc = elink->arg;

	offset = req->addr - elink->base_emul - EPW_WINDOW;

	switch (req->data_len) {
	case 8:
		val = *(uint64_t *)req->data;
		break;
	case 4:
		val = *(uint32_t *)req->data;
		break;
	case 2:
		val = *(uint16_t *)req->data;
		break;
	case 1:
		val = *(uint8_t *)req->data;
		break;
	}

	dprintf("%s: offset %lx\n", __func__, offset);

	if (elink->type == MSGDMA_CSR)
		if (req->is_write)
			csr_w(sc, req, offset, val);
		else
			csr_r(sc, req, offset);
	else
		if (req->is_write)
			pf_w(sc, req, offset, val);
		else
			pf_r(sc, req, offset);
}

static void
emul_msgdma_work(void *arg)
{
	struct msgdma_softc *sc;

	sc = arg;

	printf("%s: startup\n", __func__);

	while (1) {
		mdx_sem_wait(&sc->sem);
		emul_msgdma_poll(sc);
	}
}

int
emul_msgdma_rx_init(struct msgdma_softc *sc)
{
	struct thread *td;

	mdx_sem_init(&sc->sem, 1);

	td = mdx_thread_create("work", 1, 100000, 4096, emul_msgdma_work, sc);
	if (td == NULL)
		return (-1);

	mdx_sched_add(td);

	return (0);
}

void
emul_msgdma_fifo_intr(void *arg)
{
	struct msgdma_softc *sc;

	sc = arg;

	dprintf("%s(%d)\n", __func__, sc->unit);

	if (sc->unit == 1)
		mdx_sem_post(&sc->sem);
}
