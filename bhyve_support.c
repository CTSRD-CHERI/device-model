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
#include <sys/malloc.h>
#include <sys/lock.h>

#include "bhyve/mem.h"
#include "bhyve/inout.h"
#include "bhyve/pci_emul.h"
#include "bhyve/pci_irq.h"
#include "bhyve/pci_lpc.h"
#include "bhyve_support.h"

void
pci_irq_assert(struct pci_devinst *pi)
{

	printf("%s: pi_name %s pi_bus %d pi_slot %d pi_func %d\n",
	    __func__, pi->pi_name, pi->pi_bus, pi->pi_slot, pi->pi_func);
}

void
pci_irq_deassert(struct pci_devinst *pi)
{

	printf("%s: pi_name %s pi_bus %d pi_slot %d pi_func %d\n",
	    __func__, pi->pi_name, pi->pi_bus, pi->pi_slot, pi->pi_func);
}

#if 0
int
register_mem(struct mem_range *memp)
{

	printf("%s: name %s base %lx size %lx\n",
	    __func__, memp->name, memp->base, memp->size);

	return (0);
}

int 
unregister_mem(struct mem_range *memp)
{

	printf("%s: name %s base %lx size %lx\n",
	    __func__, memp->name, memp->base, memp->size);

	return (0);
}

int
register_mem_fallback(struct mem_range *memp)
{

	printf("%s: name %s base %lx size %lx\n",
	    __func__, memp->name, memp->base, memp->size);

	return (0);
}
#endif

int
register_inout(struct inout_port *iop)
{

	printf("%s: name %s port %d size %d\n",
	    __func__, iop->name, iop->port, iop->size);

	return (0);
}

int
unregister_inout(struct inout_port *iop)
{

	printf("%s: name %s port %d size %d\n",
	    __func__, iop->name, iop->port, iop->size);

	return (0);
}

int
pirq_alloc_pin(struct pci_devinst *pi)
{

	printf("%s: pi_name %s pi_bus %d pi_slot %d pi_func %d\n",
	    __func__, pi->pi_name, pi->pi_bus, pi->pi_slot, pi->pi_func);

	return (0);
}

int
pirq_irq(int pin)
{

	printf("%s: pin %d\n", __func__, pin);

	return (0);
}

void
lpc_pirq_routed(void)
{

	printf("%s\n", __func__);
}

void *
paddr_guest2host(struct vmctx *ctx, uintptr_t gaddr, size_t len)
{

	printf("%s: gaddr %lx len %d\n", __func__, gaddr, len);

	return (NULL);
}
