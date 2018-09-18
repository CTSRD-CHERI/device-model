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

#include <machine/cpuregs.h>

#include <mips/beri/beri_epw.h>
#include <dev/altera/msgdma/msgdma.h>

#include "device-model.h"
#include "emul_msgdma.h"

#if 0
static void
test_ipi(void)
{
	uint64_t addr;

	addr = BERIPIC0_IP_SET | MIPS_XKPHYS_UNCACHED_BASE;
	printf("Sending mSGDMA interrupt...\n");
	*(volatile uint64_t *)(addr) = (1 << 17);	/* mSGDMA0 */
	*(volatile uint64_t *)(addr) = (1 << 18);	/* mSGDMA0 */
}
#endif

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
