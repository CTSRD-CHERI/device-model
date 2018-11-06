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
#include <sys/malloc.h>

#include <machine/cpuregs.h>

#include <mips/beri/beri_epw.h>

#include "device-model.h"
#include "fwd_device.h"
#include "emul.h"
#include "emul_msgdma.h"
#include "emul_pci.h"
#include "bhyve_support.h"

#define	DM_FWD_NDEVICES		4
#define	DM_EMUL_NDEVICES	5

struct msgdma_softc msgdma0_sc;
struct msgdma_softc msgdma1_sc;
struct pci_softc pci0_sc;

#undef FWD_ENABLE
#undef MSGDMA_ENABLE

#ifdef FWD_ENABLE
const struct fwd_link fwd_map[DM_FWD_NDEVICES] = {
	{ 0x0000, 0x20, MSGDMA0_BASE_CSR,  fwd_request },	/* Control Status Register */
	{ 0x0020, 0x20, MSGDMA0_BASE_DESC, fwd_request },	/* Prefetcher */
	{ 0x0040, 0x20, MSGDMA1_BASE_CSR,  fwd_request },	/* Control Status Register */
	{ 0x0060, 0x20, MSGDMA1_BASE_DESC, fwd_request },	/* Prefetcher */
};
#endif

const struct emul_link emul_map[DM_EMUL_NDEVICES] = {
#ifdef MSGDMA_ENABLE
	{ 0x04080, 0x00020, emul_msgdma, &msgdma0_sc, MSGDMA_CSR },
	{ 0x040a0, 0x00020, emul_msgdma, &msgdma0_sc, MSGDMA_PF  },
	{ 0x04000, 0x00020, emul_msgdma, &msgdma1_sc, MSGDMA_CSR },
	{ 0x04020, 0x00020, emul_msgdma, &msgdma1_sc, MSGDMA_PF  },
#endif
	{ 0x10000, 0x50000, emul_pci, &pci0_sc, PCI_GENERIC },
};

static int
dm_request(struct epw_softc *sc, struct epw_request *req)
{
#ifdef FWD_ENABLE
	const struct fwd_link *flink;
#endif
	const struct emul_link *elink;
	uint64_t offset;
	int i;

	offset = req->addr - EPW_WINDOW;

	printf("%s: offset %lx\n", __func__, offset);

#ifdef FWD_ENABLE
	/* Check if this is forwarding request */
	for (i = 0; i < DM_FWD_NDEVICES; i++) {
		flink = &fwd_map[i];
		if (offset >= flink->base_emul &&
		    offset < (flink->base_emul + flink->size)) {
			flink->request(flink, sc, req);
			return (0);
		}
	}
#endif

	/* Check if this is emulation request */
	for (i = 0; i < DM_EMUL_NDEVICES; i++) {
		elink = &emul_map[i];
		if (offset >= elink->base_emul &&
		    offset < (elink->base_emul + elink->size)) {
			elink->request(elink, sc, req);
			return (0);
		}
	}

	printf("%s: unknown request to offset 0x%lx\n", __func__, offset);

	return (-1);
}

void
dm_init(struct epw_softc *sc)
{
	uintptr_t malloc_base;
	int malloc_size;

	msgdma0_sc.fifo_base_mem = FIFO2_BASE_MEM;
	msgdma0_sc.fifo_base_ctrl = FIFO2_BASE_CTRL;
	msgdma0_sc.unit = 0;

	msgdma1_sc.fifo_base_mem = FIFO3_BASE_MEM;
	msgdma1_sc.fifo_base_ctrl = FIFO3_BASE_CTRL;
	msgdma1_sc.unit = 1;

	malloc_base = 0xffffffffb0000000 + 0x01000000/2;
	malloc_size = 0x01000000/2;

	fl_init();   
	fl_add_region(malloc_base, malloc_size);

	emul_pci_init(&pci0_sc);
}

void
dm_loop(struct epw_softc *sc)
{
	struct epw_request req;
	int ret;

	printf("Hello World!\n");

	while (1) {
		if (epw_request(sc, &req) != 0) {
			ret = dm_request(sc, &req);
			epw_reply(sc, &req);
		}

		usleep(100000);

		emul_msgdma_poll(&msgdma0_sc);
		emul_msgdma_poll(&msgdma1_sc);
	}
}
