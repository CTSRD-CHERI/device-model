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

#include <machine/cpuregs.h>
#include <machine/cpufunc.h>
#include <machine/frame.h>
#include <machine/cache_mipsNN.h>
#include <machine/cache_r4k.h>

#include <mips/beri/beri_epw.h>
#include <dev/pci/pcireg.h>

#include "device-model.h"
#include "emul.h"
#include "emul_pci.h"

#include "bhyve/mem.h"
#include "bhyve/pci_e82545.h"
#include "bhyve/bhyve_support.h"

#define	EMUL_PCI_DEBUG
#undef	EMUL_PCI_DEBUG

#ifdef	EMUL_PCI_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

void
emul_pci(const struct emul_link *elink, struct epw_softc *epw_sc,
    struct epw_request *req)
{
	struct pci_softc *sc;
	uint64_t offset;
	uint64_t val;
	int bytes;
	int error;
	int i;
	uint8_t val8[8];
	uint32_t len;
	int bus, slot, func, coff;

	sc = elink->arg;

	KASSERT(elink->type == PCI_GENERIC, ("Unknown device"));

	offset = req->addr - elink->base_emul - EPW_WINDOW;

	coff = offset & 0xfff;
	func = (offset >> 12) & 0x7;
	slot = (offset >> 15) & 0x1f;
	bus = (offset >> 20) & 0xff;

	if (req->is_write) {
		KASSERT(req->data_len < 8,
		    ("Wrong access width %d", req->data_len));
		switch (req->data_len) {
		case 8:
			val = *(uint64_t *)req->data;
			val = bswap64(val);
			break;
		case 4:
			val = *(uint32_t *)req->data;
			val = bswap32(val);
			break;
		case 2:
			val = *(uint16_t *)req->data;
			val = bswap16(val);
			break;
		case 1:
			val = *(uint8_t *)req->data;
			break;
		}

		error = emulate_mem(sc->ctx, 0, req->addr, req->is_write,
		    req->flit_size, &val);
		dprintf("Error %d, val %lx\n", error, val);
	} else {
		bytes = req->data_len;
		error = emulate_mem(sc->ctx, 0, req->addr, req->is_write,
		    req->flit_size, (uint64_t *)&val8[0]);
		if (error == 0)
			for (i = 0; i < bytes; i++)
				/* TODO: is offset required here ? */
				req->data[8 - bytes - offset % 8 + i] =
				    val8[7 - i];
	}

	if (error == 0) {
		dprintf("%s: dev req (is_write %d) paddr %lx, val %lx\n",
		    __func__, req->is_write, req->addr, val);
		return;
	}

	if (req->is_write) {
		bytes = req->data_len;
		printf("%s (%d/%d/%d): %d-bytes write to %lx, val %lx\n",
		    __func__, bus, slot, func, bytes, offset, val);
		bcopy((void *)&req->data[0], &val8[4 - bytes], bytes);
		bhyve_pci_cfgrw(sc->ctx, 0, bus, slot, func, coff,
		    bytes, (uint32_t *)&val8[0]);
	} else {
		bzero((void *)&req->data[0], 32);
		bytes = req->flit_size;

		printf("%s (%d/%d/%d): %d-bytes read from %lx, ",
		    __func__, bus, slot, func, bytes, offset);
		bhyve_pci_cfgrw(sc->ctx, 1, bus, slot, func, coff,
		    bytes, (uint32_t *)&val8[0]);
		printf("val %x\n", *(uint32_t *)&val8[0]);

		len = bytes + offset % 8;
		bcopy((void *)&val8[4 - bytes],
		    (void *)&req->data[8 - len], bytes);
	}
}

int
emul_pci_init(struct pci_softc *sc)
{
	uint32_t val;

	sc->ctx = malloc(sizeof(struct vmctx));
	if (sc->ctx == NULL)
		return (-1);

	bhyve_pci_init(sc->ctx);

	/* Test request */
	bhyve_pci_cfgrw(sc->ctx, 1, 0, 0, 0, 0x00, 2, (uint32_t *)&val);
	printf("slot 0 val 0x%x\n", val);

	bhyve_pci_cfgrw(sc->ctx, 1, 0, 1, 0, 0x00, 2, (uint32_t *)&val);
	printf("slot 1 val 0x%x\n", val);

	return (0);
}
