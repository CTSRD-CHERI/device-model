/*-
 * Copyright (c) 2018 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-10-C-0237
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#include <machine/cpuregs.h>
#include <machine/frame.h>

#include <mips/beri/beri_epw.h>
#include <dev/altera/msgdma/msgdma.h>
#include <dev/altera/fifo/a_api.h>

#include "device-model.h"
#include "emul_msgdma.h"

#define	EMUL_MSGDMA_DEBUG
#undef	EMUL_MSGDMA_DEBUG

#ifdef	EMUL_MSGDMA_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

#define	AVALON_FIFO_TX_BASIC_OPTS_DEPTH	16

#define	SOFTDMA_RX_EVENTS	\
	(A_ONCHIP_FIFO_MEM_CORE_INTR_FULL	| \
	 A_ONCHIP_FIFO_MEM_CORE_INTR_OVERFLOW	| \
	 A_ONCHIP_FIFO_MEM_CORE_INTR_UNDERFLOW)

#define	WR4_FIFO_MEM(_sc, _reg, _val)		\
    *(volatile uint32_t *)((_sc)->fifo_base_mem + _reg) = _val
#define	RD4_FIFO_MEM(_sc, _reg)			\
    *(volatile uint32_t *)((_sc)->fifo_base_mem + _reg)

#define	WR4_FIFO_MEMC(_sc, _reg, _val)		\
    *(volatile uint32_t *)((_sc)->fifo_base_ctrl + _reg) = _val
#define	RD4_FIFO_MEMC(_sc, _reg)		\
    *(volatile uint32_t *)((_sc)->fifo_base_ctrl + _reg)

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
	uint64_t addr;

	dprintf("%s\n", __func__);

	addr = BERIPIC0_IP_SET | MIPS_XKPHYS_UNCACHED_BASE;

	if (sc->unit == 1)
		*(volatile uint64_t *)(addr) = (1 << DM_MSGDMA1_INTR);
	else
		*(volatile uint64_t *)(addr) = (1 << DM_MSGDMA0_INTR);
}

static uint32_t
emul_msgdma_fill_level(struct msgdma_softc *sc)
{
	uint32_t reg;

	reg = RD4_FIFO_MEMC(sc,
	    A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_FILL_LEVEL);

	return (le32toh(reg));
}

static uint32_t
emul_msgdma_fill_level_wait(struct msgdma_softc *sc)
{
	uint32_t val;

	do {
		val = emul_msgdma_fill_level(sc);
	} while (val == AVALON_FIFO_TX_BASIC_OPTS_DEPTH);

	return (val);
}

static void
emul_msgdma_process_rx(struct msgdma_softc *sc)
{
	struct msgdma_desc *desc;
	uint32_t transferred;
	uint32_t fill_level;
	uint64_t read_lo;
	uint64_t write_lo;
	uint32_t len;
	uint32_t control;
	uint32_t data;
	uint32_t meta;
	uint32_t timeout;
	int error;
	int sop_rcvd;
	int eop_rcvd;
	int empty;

	fill_level = emul_msgdma_fill_level(sc);
	if (fill_level == 0)
		return;

	printf("%s(%d): fill_level %d\n", __func__, sc->unit, fill_level);

	desc = sc->cur_desc;

	control = le32toh(desc->control);
	if ((control & CONTROL_OWN) == 0)
		return;

	read_lo = le32toh(desc->read_lo);
	write_lo = le32toh(desc->write_lo);
	len = le32toh(desc->length);

	printf("%s: desc %x -> %x, %d bytes\n", __func__, read_lo, write_lo, len);
	write_lo |= MIPS_XKPHYS_UNCACHED_BASE;
	error = 0;
	sop_rcvd = 0;
	eop_rcvd = 0;
	empty = 0;
	transferred = 0;
	while (fill_level) {
		data = RD4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_DATA);
		meta = RD4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_METADATA);
		meta = le32toh(meta);
		if (meta & A_ONCHIP_FIFO_MEM_CORE_ERROR_MASK) {
			error = 1;
			break;
		}

		if ((meta & A_ONCHIP_FIFO_MEM_CORE_CHANNEL_MASK) != 0) {
			error = 1;
			break;
		}

		if (meta & A_ONCHIP_FIFO_MEM_CORE_SOP)
			sop_rcvd = 1;

		if (meta & A_ONCHIP_FIFO_MEM_CORE_EOP) {
			empty = (meta & A_ONCHIP_FIFO_MEM_CORE_EMPTY_MASK) >>
			    A_ONCHIP_FIFO_MEM_CORE_EMPTY_SHIFT;
		}

		if (sop_rcvd == 0) {
			printf("%s: sop not received\n", __func__);
			error = 1;
			break;
		}

		if (empty == 0) {
			*(uint32_t *)(write_lo) = data;
			write_lo += 4;
			transferred += 4;
		} else if (empty == 1) {
			*(uint16_t *)(write_lo) = ((data >> 16) & 0xffff);
			write_lo += 2;
			transferred += 2;
			*(uint8_t *)(write_lo) = ((data >> 8) & 0xff);
			write_lo += 1;
			transferred += 1;
		} else {
			panic("implement me %d\n", empty);
		}

		if (meta & A_ONCHIP_FIFO_MEM_CORE_EOP) {
			eop_rcvd = 1;
			break;
		}

		fill_level = emul_msgdma_fill_level(sc);
		timeout = 10000;
		while (fill_level == 0 && timeout--)
			fill_level = emul_msgdma_fill_level(sc);
		if (timeout == 0) {
			printf("%s: eop not received\n", __func__);
			/* No EOP received. Broken packet. */
			error = 1;
			break;
		}
	}

	if (error == 0) {
		printf("%s: packet received, %d bytes (sop_rcvd %d eop_rcvd %d)\n", __func__, transferred, sop_rcvd, eop_rcvd);

		desc->transferred = htole32(transferred);
		control &= ~CONTROL_OWN;
		desc->control = htole32(control);
		__asm __volatile("sync;sync;sync");
		sc->cur_desc = emul_msgdma_next_desc(desc);

		send_soft_irq(sc);
	}
}

void
emul_msgdma_fifo_intr(void *arg)
{
	struct msgdma_softc *sc;
	uint32_t reg;
	uint32_t err;

	sc = arg;

	reg = RD4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_EVENT);
	reg = le32toh(reg);

	printf("%s: reg %x\n", __func__, reg);

	if (reg & (A_ONCHIP_FIFO_MEM_CORE_EVENT_OVERFLOW |
	    A_ONCHIP_FIFO_MEM_CORE_EVENT_UNDERFLOW)) {
		/* Errors */
		err = (((reg & A_ONCHIP_FIFO_MEM_CORE_ERROR_MASK) >> \
		    A_ONCHIP_FIFO_MEM_CORE_ERROR_SHIFT) & 0xff);
		}

	//if (reg != 0) {
		WR4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_EVENT, htole32(reg));
		emul_msgdma_process_rx(sc);
	//}
}

static void
emul_msgdma_process_tx(struct msgdma_softc *sc,
    struct msgdma_desc *desc)
{
	uint32_t transferred;
	uint32_t control;
	uint64_t write_lo;
	uint64_t read_lo;
	uint32_t len;
	uint32_t reg;
	uint32_t val;

	control = le32toh(desc->control);
	read_lo = le32toh(desc->read_lo);
	write_lo = le32toh(desc->write_lo);
	len = le32toh(desc->length);
	printf("%s: copy %x -> %x, %d bytes\n", __func__, read_lo, write_lo, len);
	read_lo |= MIPS_XKPHYS_UNCACHED_BASE;

	uint32_t fill_level;
	uint32_t c;
	uint32_t leftm;
	uint32_t tmp;

	transferred = 0;

	if (write_lo != 0)
		panic("error\n");

	fill_level = emul_msgdma_fill_level_wait(sc);

	/* Mem to dev */
	if (control & CONTROL_GEN_SOP) {
		reg = A_ONCHIP_FIFO_MEM_CORE_SOP;
		WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_METADATA, htole32(reg));
	}

	c = 0;
	while ((len - c) >= 4) {
		val = *(uint32_t *)(read_lo);
		WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_DATA, val);

		fill_level = emul_msgdma_fill_level_wait(sc);

		read_lo += 4;
		transferred += 4;
		c += 4;
	}

	val = 0;
	leftm = (len - c);

	switch (leftm) {
	case 1:
		val = *(uint8_t *)(read_lo);
		val <<= 24;
		read_lo += 1;
		transferred += 1;
		break;
	case 2:
	case 3:
		val = *(uint16_t *)(read_lo);
		val <<= 16;
		read_lo += 2;
		transferred += 2;
		if (leftm == 3) {
			tmp = *(uint8_t *)(read_lo);
			val |= (tmp << 8);
			read_lo += 1;
			transferred += 1;
		}
	}

	/* Set end of packet. */
	reg = 0;
	if (control & CONTROL_GEN_EOP)
		reg |= A_ONCHIP_FIFO_MEM_CORE_EOP;
	reg |= ((4 - leftm) << A_ONCHIP_FIFO_MEM_CORE_EMPTY_SHIFT);
	WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_METADATA, htole32(reg));

	/* Ensure there is a FIFO entry available. */
	fill_level = emul_msgdma_fill_level_wait(sc);

	/* Final write */
	WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_DATA, val);

	printf("%s: packet sent, %d bytes\n", __func__, transferred);
	desc->transferred = htole32(transferred);
	control &= ~CONTROL_OWN;
	desc->control = htole32(control);
	__asm __volatile("sync;sync;sync");

	sc->cur_desc = emul_msgdma_next_desc(desc);
	send_soft_irq(sc);
}

void
emul_msgdma_poll(struct msgdma_softc *sc)
{
	struct msgdma_desc *desc;
	uint32_t reg;

	if (sc->poll_en == 0)
		return;

	if (sc->unit == 1) {
		emul_msgdma_process_rx(sc);
		return;
	}

	desc = sc->cur_desc;
	reg = le32toh(desc->control);
	if (reg & CONTROL_OWN) {
		printf("%s(%d): desc->control %x\n", __func__, sc->unit, reg);
		emul_msgdma_process_tx(sc, desc);
	} else {
		printf("%s(%d): not owned: %x\n", __func__, sc->unit, reg);
	}
}

static int
emul_msgdma_poll_enable(struct msgdma_softc *sc)
{
	uint64_t addr;

	if (sc->pf_next_lo == 0)
		return (-1);

	addr = sc->pf_next_lo | MIPS_XKPHYS_UNCACHED_BASE;

	sc->cur_desc = (struct msgdma_desc *)addr;

	if (sc->unit == 0)
		sc->poll_en = 1;
	else {
		sc->poll_en = 1;
		WR4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_INT_ENABLE, 0);
		return (0);
		printf("%s(%d): enabling interrupts\n", __func__, sc->unit);
		WR4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_INT_ENABLE,
		    htole32(SOFTDMA_RX_EVENTS));
	}

	return (0);
}

static void
csr_r(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset)
{

	switch (offset) {
	case DMA_STATUS:
		printf("%s: DMA_STATUS\n", __func__);
		break;
	case DMA_CONTROL:
		printf("%s: DMA_CONTROL\n", __func__);
		break;
	};
}

static void
csr_w(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset, uint64_t val)
{

	switch (offset) {
	case DMA_STATUS:
		printf("%s: DMA_STATUS, val %x\n", __func__, val);
		break;
	case DMA_CONTROL:
		printf("%s: DMA_CONTROL, val %x\n", __func__, val);
		break;
	};
}

static void
pf_r(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset)
{

	switch (offset) {
	case PF_CONTROL:
		printf("%s: PF_CONTROL\n", __func__);
		break;
	case PF_NEXT_LO:
		printf("%s: PF_NEXT_LO\n", __func__);
		bcopy((void *)&sc->pf_next_lo, (void *)req->data, 4);
		break;
	case PF_NEXT_HI:
		printf("%s: PF_NEXT_HI\n", __func__);
		bcopy(&sc->pf_next_hi, req->data, 4);
		break;
	case PF_POLL_FREQ:
		printf("%s: PF_POLL_FREQ\n", __func__);
		break;
	case PF_STATUS:
		printf("%s: PF_STATUS\n", __func__);
		break;
	};
}

static void
pf_w(struct msgdma_softc *sc, struct epw_request *req,
    uint64_t offset, uint64_t val)
{

	switch (offset) {
	case PF_CONTROL:
		printf("%s: PF_CONTROL val %lx\n", __func__, val);
		break;
	case PF_NEXT_LO:
		printf("%s: PF_NEXT_LO val %lx\n", __func__, val);
		sc->pf_next_lo = val;
		emul_msgdma_poll_enable(sc);
		break;
	case PF_NEXT_HI:
		printf("%s: PF_NEXT_HI val %lx\n", __func__, val);
		sc->pf_next_hi = val;
		break;
	case PF_POLL_FREQ:
		printf("%s: PF_POLL_FREQ val %lx\n", __func__, val);
		break;
	case PF_STATUS:
		printf("%s: PF_STATUS val %lx\n", __func__, val);
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

	printf("%s: offset %lx\n", __func__, offset);

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
