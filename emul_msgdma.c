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

#define	AVALON_FIFO_TX_BASIC_OPTS_DEPTH	16

#define	SOFTDMA_RX_EVENTS	\
	(A_ONCHIP_FIFO_MEM_CORE_INTR_FULL	| \
	 A_ONCHIP_FIFO_MEM_CORE_INTR_OVERFLOW	| \
	 A_ONCHIP_FIFO_MEM_CORE_INTR_UNDERFLOW)

#define	WR4_FIFO_MEM(_sc, _reg, _val)		\
    *(volatile uint32_t *)((_sc)->fifo_base_mem + _reg) = _val

#define	WR4_FIFO_MEMC(_sc, _reg, _val)		\
    *(volatile uint32_t *)((_sc)->fifo_base_ctrl + _reg) = _val
#define	RD4_FIFO_MEMC(_sc, _reg)		\
    *(volatile uint32_t *)((_sc)->fifo_base_ctrl + _reg)

#if 0
static void
test_ipi(void)
{
	uint64_t addr;

	addr = BERIPIC0_IP_SET | MIPS_XKPHYS_UNCACHED_BASE;
	printf("Sending mSGDMA interrupt...\n");
	*(volatile uint64_t *)(addr) = (1 << 17);	/* mSGDMA0 */
	*(volatile uint64_t *)(addr) = (1 << 18);	/* mSGDMA1 */
}
#endif

static void
emul_msgdma_process_rx(struct msgdma_softc *sc)
{

	printf("%s(%d)\n", __func__, sc->unit);
}

void
emul_msgdma_fifo_intr(void *arg)
{
	struct msgdma_softc *sc;
	uint32_t reg;
	uint32_t err;

	sc = arg;

	printf("%s\n", __func__);

	reg = RD4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_FILL_LEVEL);
	reg = le32toh(reg);
	if (reg & (A_ONCHIP_FIFO_MEM_CORE_EVENT_OVERFLOW |
	    A_ONCHIP_FIFO_MEM_CORE_EVENT_UNDERFLOW)) {
		/* Errors */
		err = (((reg & A_ONCHIP_FIFO_MEM_CORE_ERROR_MASK) >> \
		    A_ONCHIP_FIFO_MEM_CORE_ERROR_SHIFT) & 0xff);
		}

	if (reg != 0) {
		WR4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_EVENT, htole32(reg));
		emul_msgdma_process_rx(sc);
	}
}

static void
emul_msgdma_process_tx(struct msgdma_softc *sc,
    struct msgdma_desc *desc)
{
	uint32_t control;
	uint64_t write_lo;
	uint64_t read_lo;
	uint32_t len;
	uint32_t reg;
	uint32_t val;

	control = bswap32(desc->control);
	read_lo = bswap32(desc->read_lo);
	write_lo = bswap32(desc->write_lo);
	len = bswap32(desc->length);
	printf("%s: copy %x -> %x, %d bytes\n", __func__, read_lo, write_lo, len);
	read_lo |= MIPS_XKPHYS_UNCACHED_BASE;

	uint32_t fill_level;
	uint32_t c;
	uint32_t leftm;
	uint32_t tmp;

	if (write_lo == 0) {
		do {
			fill_level = RD4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_FILL_LEVEL);
			fill_level = le32toh(fill_level);
		} while (fill_level == AVALON_FIFO_TX_BASIC_OPTS_DEPTH);

		/* Mem to dev */
		if (control & CONTROL_GEN_SOP) {
			reg = A_ONCHIP_FIFO_MEM_CORE_SOP;
			WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_METADATA, htole32(reg));
		}

		c = 0;
		while ((len - c) >= 4) {
			val = *(uint32_t *)(read_lo);
			printf("writing %x\n", val);
			WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_DATA, val);

			do {
				fill_level = RD4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_FILL_LEVEL);
				fill_level = le32toh(fill_level);
			} while (fill_level == AVALON_FIFO_TX_BASIC_OPTS_DEPTH);

			read_lo += 4;
			c += 4;
		}

		val = 0;
		leftm = (len - c);

		switch (leftm) {
		case 1:
			val = *(uint8_t *)(read_lo);
			val <<= 24;
			read_lo += 1;
			break;
		case 2:
		case 3:
			val = *(uint16_t *)(read_lo);
			val <<= 16;
			read_lo += 2;
			if (leftm == 3) {
				tmp = *(uint8_t *)(read_lo);
				val |= (tmp << 8);
				read_lo += 1;
			}
		}

		/* Set end of packet. */
		reg = 0;
		if (control & CONTROL_GEN_EOP)
			reg |= A_ONCHIP_FIFO_MEM_CORE_EOP;
		reg |= ((4 - leftm) << A_ONCHIP_FIFO_MEM_CORE_EMPTY_SHIFT);
		WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_METADATA, htole32(reg));

		/* Ensure there is a FIFO entry available. */
		do {
			fill_level = RD4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_FILL_LEVEL);
			fill_level = le32toh(fill_level);
		} while (fill_level == AVALON_FIFO_TX_BASIC_OPTS_DEPTH);

		/* Final write */
		printf("final write %x\n", val);
		WR4_FIFO_MEM(sc, A_ONCHIP_FIFO_MEM_CORE_DATA, val);

		uint64_t addr;
		addr = bswap32(desc->next);
		addr |= MIPS_XKPHYS_UNCACHED_BASE;
		sc->cur_tx_desc = (struct msgdma_desc *)addr;
	}
}

void
emul_msgdma_poll(struct msgdma_softc *sc)
{
	struct msgdma_desc *desc;
	uint32_t reg;

	if (sc->poll_en == 0)
		return;

	desc = sc->cur_tx_desc;
	reg = bswap32(desc->control);
	if (reg & CONTROL_OWN) {
		printf("%s(%d): desc->control %x\n", __func__, sc->unit, reg);
		emul_msgdma_process_tx(sc, desc);
	}

}

static int
emul_msgdma_poll_enable(struct msgdma_softc *sc)
{
	uint64_t addr;

	if (sc->pf_next_lo == 0)
		return (-1);

	addr = sc->pf_next_lo | MIPS_XKPHYS_UNCACHED_BASE;

	sc->cur_tx_desc = (struct msgdma_desc *)addr;
	sc->poll_en = 1;

	if (sc->unit == 1)
		WR4_FIFO_MEMC(sc, A_ONCHIP_FIFO_MEM_CORE_STATUS_REG_INT_ENABLE,
		    htole32(SOFTDMA_RX_EVENTS));

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
