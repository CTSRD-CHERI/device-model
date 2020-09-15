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
#include <sys/malloc.h>
#include <sys/cheri.h>

#include <machine/frame.h>
#include <machine/cpuregs.h>

#include <mips/beri/beri_epw.h>
#include <mips/beri/beripic.h>

#include "mem.h"
#include "inout.h"
#include "pci_emul.h"
#include "pci_irq.h"
#include "pci_lpc.h"
#include "bhyve_support.h"

#include "device-model.h"

#define	BS_DEBUG
#undef	BS_DEBUG

#ifdef	BS_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

extern struct mdx_device beripic0;

void
pci_irq_assert(struct pci_devinst *pi)
{
	uint32_t irq;

	dprintf("%s: pi_name %s pi_bus %d pi_slot %d pi_func %d\n",
	    __func__, pi->pi_name, pi->pi_bus, pi->pi_slot, pi->pi_func);

	irq = 0;

	if (strcmp(pi->pi_name, "ahci-hd-pci-1") == 0)
		irq = DM_AHCI_INTR;
	else if (strcmp(pi->pi_name, "e1000-pci-0") == 0)
		irq = DM_E1000_INTR;

	if (irq)
		beripic_ip_set(&beripic0, irq);
}

void
pci_irq_deassert(struct pci_devinst *pi)
{
	uint32_t irq;

	dprintf("%s: pi_name %s pi_bus %d pi_slot %d pi_func %d\n",
	    __func__, pi->pi_name, pi->pi_bus, pi->pi_slot, pi->pi_func);

	irq = 0;

	if (strcmp(pi->pi_name, "ahci-hd-pci-1") == 0)
		irq = DM_AHCI_INTR;
	else if (strcmp(pi->pi_name, "e1000-pci-0") == 0)
		irq = DM_E1000_INTR;

	if (irq)
		beripic_ip_clear(&beripic0, irq);
}

int
register_inout(struct inout_port *iop)
{

	dprintf("%s: name %s port %d size %d\n",
	    __func__, iop->name, iop->port, iop->size);

	return (0);
}

int
unregister_inout(struct inout_port *iop)
{

	dprintf("%s: name %s port %d size %d\n",
	    __func__, iop->name, iop->port, iop->size);

	return (0);
}

int
pirq_alloc_pin(struct pci_devinst *pi)
{
	int pin;

	dprintf("%s: pi_name %s pi_bus %d pi_slot %d pi_func %d\n",
	    __func__, pi->pi_name, pi->pi_bus, pi->pi_slot, pi->pi_func);

	/*
	 * e1000-pci-0
	 * ahci-hd-pci-1
	 */

	pin = 1;

	return (pin);
}

int
pirq_irq(int pin)
{

	dprintf("%s: pin %d\n", __func__, pin);

	return (0);
}

void
lpc_pirq_routed(void)
{

	dprintf("%s\n", __func__);
}

void *
paddr_guest2host(struct vmctx *ctx, uintptr_t gaddr, size_t len)
{
	uintptr_t addr;
	void *result;

#ifdef CONFIG_IOMMU
	addr = gaddr;
#else
	addr = gaddr | MIPS_XKPHYS_UNCACHED_BASE;
#endif

	dprintf("%s: gaddr %lx, addr %lx, len %d\n",
	    __func__, gaddr, addr, len);

#ifdef __CHERI_PURE_CAPABILITY__
	result = cheri_setoffset(ctx->cap, addr);
#else
	result = (void *)addr;
#endif

	return (result);
}

void * __capability
cap_guest2host(struct vmctx *ctx, void * __capability gaddr, size_t len)
{

	return (gaddr);
}
