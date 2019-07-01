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

#include <machine/regdef.h>
#include <machine/asm.h>
#include <machine/cpuregs.h>
#include <mips/mips/saverestore.S>

#define	PAGE_SHIFT	12
#define	NPTEPGSHIFT	9
#define	NPDEPGSHIFT	9
#define	SEGSHIFT	(PAGE_SHIFT + NPTEPGSHIFT + NPDEPGSHIFT)
#define	PTRSHIFT	3
#define	PDEPTRMASK	0xff8
#define	PDRSHIFT	(PAGE_SHIFT + NPTEPGSHIFT)
#define	PTESHIFT	3
#define	PTESIZE		8
#define CLEAR_PTE_SWBITS(pr)
#define	PTE_MTC0	dmtc0
#define COP0_SYNC	ssnop; ssnop; ssnop; ssnop; .word 0xc0;
#define	HAZARD_DELAY	nop;nop;nop;nop;sll $0,$0,3;
#define	PTE2MASK	0xff0   /* for the 2-page lo0/lo1 */

VECTOR(MipsTLBMiss)
	.set push
	.set noat
	mfc0		k0, CP0_BAD_VADDR		# k0 = bad addr
	lui		k1, %hi(kernel_segmap)
	PTR_L		k1, %lo(kernel_segmap)(k1)
	PTR_SRL		k0, SEGSHIFT - PTRSHIFT		# k0 = seg offs (almst)
	andi		k0, k0, PDEPTRMASK		# k0 = seg offs
	PTR_ADDU	k1, k0, k1			# k1 = seg entry offs
	PTR_L		k1, 0(k1)			# k1 = seg entry
	beq		k1, zero, tlb_nosegtab0
	nop
	mfc0		k0, CP0_BAD_VADDR		# k0 = bad addr
	PTR_SRL		k0, PDRSHIFT - PTRSHIFT		# k0 = vpn
	andi		k0, k0, PDEPTRMASK		# k0 = pde offset
	PTR_ADDU	k1, k0, k1			# k1 = pde entry offs
	PTR_L		k1, 0(k1)			# k1 = pde entry
	beq		k1, zero, tlb_nosegtab1
	nop
	mfc0		k0, CP0_BAD_VADDR		# k0 = bad addr
	PTR_SRL		k0, PAGE_SHIFT - PTESHIFT	# k0 = vpn
	andi		k0, k0, PTE2MASK		# k0 = page tab offs
	PTR_ADDU	k1, k1, k0			# k1 = pte address
	REG_L		k0, 0(k1)			# k0 = lo0 pte
	REG_L		k1, PTESIZE(k1)			# k1 = lo1 pte
	CLEAR_PTE_SWBITS(k0)
	PTE_MTC0	k0, CP0_TLB_LO0
	COP0_SYNC
	CLEAR_PTE_SWBITS(k1)
	PTE_MTC0	k1, CP0_TLB_LO1
	COP0_SYNC
	tlbwr
	HAZARD_DELAY
	eret
	nop
	.set pop
VECTOR_END(MipsTLBMiss)

VECTOR(MipsTLBMissException)
	.set push
	.set noat
	mfc0		k0, CP0_BAD_VADDR		# k0 = bad addr
	lui		k1, %hi(kernel_segmap)
	PTR_L		k1, %lo(kernel_segmap)(k1)
	PTR_SRL		k0, SEGSHIFT - PTRSHIFT		# k0 = seg offs (almst)
	andi		k0, k0, PDEPTRMASK		# k0 = seg offs
	PTR_ADDU	k1, k0, k1			# k1 = seg entry offs
	PTR_L		k1, 0(k1)			# k1 = seg entry
	beq		k1, zero, tlb_nosegtab0
	nop
	mfc0		k0, CP0_BAD_VADDR		# k0 = bad addr
	PTR_SRL		k0, PDRSHIFT - PTRSHIFT		# k0 = vpn
	andi		k0, k0, PDEPTRMASK		# k0 = pde offset
	PTR_ADDU	k1, k0, k1			# k1 = pde entry offs
	PTR_L		k1, 0(k1)			# k1 = pde entry
	beq		k1, zero, tlb_nosegtab1
	nop
	mfc0		k0, CP0_BAD_VADDR		# k0 = bad addr
	PTR_SRL		k0, PAGE_SHIFT - PTESHIFT	# k0 = vpn
	andi		k0, k0, PTE2MASK		# k0 = page tab offs
	PTR_ADDU	k1, k1, k0			# k1 = pte address
	REG_L		k0, 0(k1)			# k0 = lo0 pte
	REG_L		k1, PTESIZE(k1)			# k1 = lo1 pte
	CLEAR_PTE_SWBITS(k0)
	PTE_MTC0	k0, CP0_TLB_LO0
	COP0_SYNC
	CLEAR_PTE_SWBITS(k1)
	PTE_MTC0	k1, CP0_TLB_LO1
	COP0_SYNC
	tlbwr
	HAZARD_DELAY
	jr	ra
	nop
	.set pop
VECTOR_END(MipsTLBMissException)
