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

#ifndef	_DEVICE_MODEL_H_
#define	_DEVICE_MODEL_H_

#include <mips/beri/beri_epw.h>

#define	AJU0_BASE		0xff7f000000
#define	AJU1_BASE		0xff7f001000
#define	AJU2_BASE		0xff7f002000
#define	EPW_BASE		0xff7ff00000	/* Control interface */
#define	EPW_WINDOW		0xfdb00000	/* Virtual device window */

#define	MSGDMA0_BASE_CSR	0x80004080
#define	MSGDMA0_BASE_DESC	0x800040a0
#define	MSGDMA1_BASE_CSR	0x80004000
#define	MSGDMA1_BASE_DESC	0x80004020

#define	DM_MSGDMA0_BASE_CSR	0xfdb04080
#define	DM_MSGDMA0_BASE_DESC	0xfdb040a0
#define	DM_MSGDMA0_INTR		18

#define	DM_MSGDMA1_BASE_CSR	0xfdb04000
#define	DM_MSGDMA1_BASE_DESC	0xfdb04020
#define	DM_MSGDMA1_INTR		17

#define	DM_E1000_INTR		19
#define	DM_AHCI_INTR		20

#define	FIFO0_BASE_MEM		0xff7f007400
#define	FIFO0_BASE_CTRL		0xff7f007420
#define	FIFO0_INTR		2
#define	FIFO1_BASE_MEM		0xff7f007500
#define	FIFO1_BASE_CTRL		0xff7f007520
#define	FIFO1_INTR		1
#define	FIFO2_BASE_MEM		0xff7f005400
#define	FIFO2_BASE_CTRL		0xff7f005420
#define	FIFO2_INTR		12
#define	FIFO3_BASE_MEM		0xff7f005500
#define	FIFO3_BASE_CTRL		0xff7f005520
#define	FIFO3_INTR		11

#define	BERIPIC0_CFG		0xff7f804000
#define	BERIPIC0_IP_READ	0xff7f806000
#define	BERIPIC0_IP_SET		0xff7f806080
#define	BERIPIC0_IP_CLEAR	0xff7f806100

#define	BERIPIC1_CFG		0xff7f808000
#define	BERIPIC1_IP_READ	0xff7f80a000
#define	BERIPIC1_IP_SET		0xff7f80a080
#define	BERIPIC1_IP_CLEAR	0xff7f80a100

#define	MIPS_DEFAULT_FREQ	100000000 /* 100 MHz */
#define	USEC_TO_TICKS(n)	((n) * 100)

void cpu_reset(void);

void dm_loop(struct epw_softc *sc);
void dm_init(struct epw_softc *sc);

#endif	/* !_DEVICE_MODEL_H_ */
