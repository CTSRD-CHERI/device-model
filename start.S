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

#include <machine/regdef.h>
#include <machine/cpuregs.h>
#include <machine/asm.h>

#include <mips/mips/saverestore.S>

#define	BERI_RESET_VECTOR	0x40000000

ASM_ENTRY(_start)
	nop
	/* Debug */
	dla	t0, (DM_BASE + 0x800000)
	dli	t1, 0x1212121212131313
	sd	t1, 0(t0)
	nop
	sync
	sync
	sync
	sync
	sync

	/* Zero BSS */
	li	t2, 0
	dla	t0, _sbss
	nop
	dla	t1, _ebss
	nop
1:
	beq	t0, t1, 2f
	nop
	sb	t2, 0(t0)
	addi	t0, t0, 1
	j	1b
	nop
2:

#if 0
	/* Debug: output a char to AJU1 console. */
1:
	dli	t0, 0x900000007f001000
	dli	t1, 0x6100000000000000
	sd	t1, 0(t0)
	nop
	j	1b
	nop
#endif

	dla	sp, (DM_BASE + 0x100000)
	nop
	li	a0, 0
	dla	t0, md_init
	jalr	t0
	nop

ENTRY(cpu_reset)

	/* Reset */
	dla	t0, (BERI_RESET_VECTOR | MIPS_XKPHYS_UNCACHED_BASE)
	jr	t0
	nop

	/* NOT REACHED */
1:
	j	1b
	nop
END(cpu_reset)
