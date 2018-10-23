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
#include <machine/cpufunc.h>
#include <machine/frame.h>
#include <machine/cache_mipsNN.h>
#include <machine/cache_r4k.h>

#include <mips/beri/beri_epw.h>
#include <dev/pci/pcireg.h>

#include "device-model.h"
#include "emul.h"
#include "emul_pci.h"

#define	EMUL_PCI_DEBUG
#undef	EMUL_PCI_DEBUG

#ifdef	EMUL_PCI_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

static void
emul_pci_write(struct pci_softc *sc, struct epw_request *req,
    uint64_t offset, uint64_t val)
{

	dprintf("%s: write to %lx val %lx\n", __func__, offset, val);

	switch (offset) {
	case PCIR_COMMAND:
		sc->cmd = val;
		break;
	}
}

static void
emul_pci_read(struct pci_softc *sc, struct epw_request *req,
    uint64_t offset)
{
	uint16_t val;

	dprintf("%s: read from %lx\n", __func__, offset);

	switch (offset) {
	case PCIR_VENDOR:
		val = 0x8086;
		bcopy((void *)&val, (void *)&req->data[6], 2);

		break;
	case PCIR_DEVICE:
		val = 0x100F;
		bcopy((void *)&val, (void *)&req->data[4], 2);
#if 0
		req->data[0] = 1;
		req->data[1] = 2;
		req->data[2] = 3;
		req->data[3] = 4;
		req->data[4] = 5;
		req->data[5] = 6;
		req->data[6] = 7;
		req->data[7] = 8;
		req->data[8] = 9;
#endif
		break;
	case PCIR_COMMAND:
		bcopy((void *)&sc->cmd, (void *)&req->data[2], 2);
		break;
	case PCIR_STATUS:
	case PCIR_REVID:
	case PCIR_PROGIF:
	case PCIR_SUBCLASS:
	case PCIR_CLASS:
	case PCIR_CACHELNSZ:
	case PCIR_LATTIMER:
	case PCIR_HDRTYPE:
	case PCIR_BIST:
		break;
	}
}

void
emul_pci(const struct emul_link *elink, struct epw_softc *epw_sc,
    struct epw_request *req)
{
	struct pci_softc *sc;
	uint64_t offset;
	uint64_t val;

	sc = elink->arg;

	KASSERT(elink->type == PCI_GENERIC, ("Unknown device"));

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

	if (req->is_write)
		emul_pci_write(sc, req, offset, val);
	else
		emul_pci_read(sc, req, offset);
}
