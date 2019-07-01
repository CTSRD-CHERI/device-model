/*-
 * Copyright (c) 2019 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
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
#include <sys/thread.h>

#include <machine/frame.h>
#include <machine/cpuregs.h>
#include <machine/cpufunc.h>

#include <mips/mips/trap.h>

#include "tlb.h"

#define	PAGE_SHIFT		12
#define	PAGE_SIZE		(1 << PAGE_SHIFT)
#define	PAGE_MASK		(PAGE_SIZE - 1)
#define	TLB_PAGE_SHIFT		PAGE_SHIFT

#define	TLBHI_ASID_MASK		(0xff)
#define	TLBHI_R_SHIFT		62
#define	TLBHI_R_USER		(0x00UL << TLBHI_R_SHIFT)
#define	TLBHI_R_SUPERVISOR	(0x01UL << TLBHI_R_SHIFT)
#define	TLBHI_R_KERNEL		(0x03UL << TLBHI_R_SHIFT)
#define	TLBHI_R_MASK		(0x03UL << TLBHI_R_SHIFT)
#define	TLBHI_VA_R(va)		((va) & TLBHI_R_MASK)
#define	TLBHI_FILL_SHIFT	40
#define	TLBHI_VPN2_SHIFT	(TLB_PAGE_SHIFT + 1)
#define	TLBHI_VPN2_MASK		(((~((1UL << TLBHI_VPN2_SHIFT) - 1)) << (63 - TLBHI_FILL_SHIFT)) >> (63 - TLBHI_FILL_SHIFT))
#define	TLBHI_VA_TO_VPN2(va)	((va) & TLBHI_VPN2_MASK)
#define	TLBHI_ENTRY(va, asid)	((TLBHI_VA_R((va))) /* Region. */ | \
				 (TLBHI_VA_TO_VPN2((va))) /* VPN2. */ | \
				 ((asid) & TLBHI_ASID_MASK))

#define	COP0_SYNC  ssnop; ssnop; ssnop; ssnop; .word 0xc0;

static __inline void
mips_cp0_sync(void)
{
	__asm __volatile ("ssnop; ssnop; ssnop; ssnop; .word 0xc0;");
}

static inline void
tlb_probe(void)
{
	__asm __volatile ("tlbp" : : : "memory");
	mips_cp0_sync();
}

#if 0
static inline void
tlb_read(void)
{
	__asm __volatile ("tlbr" : : : "memory");
	mips_cp0_sync();
}
#endif

static inline void
tlb_write_indexed(void)
{
	__asm __volatile ("tlbwi" : : : "memory");
	mips_cp0_sync();
}

void
tlb_invalidate_address(vm_offset_t va)
{
	register_t s;
	int i;

	va &= ~PAGE_MASK;

	s = intr_disable();

	mips_wr_pagemask(0);
	mips_wr_entryhi(TLBHI_ENTRY(va, 0));
	tlb_probe();
	i = mips_rd_index();
	if (i >= 0) {
		mips_wr_entrylo0(0);
		mips_cp0_sync();
		mips_wr_entrylo1(0);
		mips_cp0_sync();
		mips_wr_pagemask(0);
		mips_wr_index(i);
		tlb_write_indexed();
		mips_cp0_sync();
	}

	intr_restore(s);
}
