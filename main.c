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
#include <sys/console.h>
#include <sys/endian.h>
#include <sys/thread.h>
#include <sys/systm.h>

#include <machine/frame.h>
#include <machine/cpuregs.h>
#include <machine/cpufunc.h>

#include <mips/mips/timer.h>
#include <mips/beri/beripic.h>
#include <mips/beri/beri_epw.h>
#include <dev/altera/jtag_uart/jtag_uart.h>
#include <dev/altera/fifo/fifo.h>

#include <mips/mips/trap.h>

#include "device-model.h"
#include "fwd_device.h"
#include "emul.h"
#include "emul_msgdma.h"
#include "test.h"

struct beripic_resource beripic1_res = {
	.cfg = BERIPIC1_CFG,
	.ip_read = BERIPIC1_IP_READ,
	.ip_set = BERIPIC1_IP_SET,
	.ip_clear = BERIPIC1_IP_CLEAR,
};

static struct aju_softc aju_sc;
static struct beripic_softc beripic_sc;
static struct epw_softc epw_sc;
static struct mips_timer_softc timer_sc;

extern struct altera_fifo_softc fifo0_sc;
extern struct altera_fifo_softc fifo1_sc;

static void
softintr(void *arg, struct trapframe *frame, int i)
{
	uint32_t cause;

	printf("Soft interrupt %d\n", i);

	cause = mips_rd_cause();
	cause &= ~(1 << (8 + i));
	mips_wr_cause(cause);
};

static void
hardintr(void *arg, struct trapframe *frame, int i)
{

	printf("Unknown hard interrupt %d\n", i);
}

static void
ipi_from_freebsd(void *arg)
{

	printf("%s: cpu_reset\n", __func__);

	cpu_reset();
}

static const struct mips_intr_entry mips_intr_map[MIPS_N_INTR] = {
	[0] = { softintr, NULL },
	[1] = { softintr, NULL },
	[2] = { beripic_intr, (void *)&beripic_sc },
	[3] = { hardintr, NULL },
	[4] = { hardintr, NULL },
	[5] = { hardintr, NULL },
	[6] = { hardintr, NULL },
	[7] = { mips_timer_intr, (void *)&timer_sc },
};

static const struct beripic_intr_entry beripic_intr_map[BERIPIC_NIRQS] = {
	[11] = { altera_fifo_intr, (void *)&fifo1_sc },	/* rx */
	[12] = { altera_fifo_intr, (void *)&fifo0_sc },	/* tx */
	[16] = { ipi_from_freebsd, NULL },
};

static void
uart_putchar(int c, void *arg)
{
	struct aju_softc *sc;

	sc = arg;

	if (c == '\n')
		aju_putc(sc, '\r');

	aju_putc(sc, c);
}

void
udelay(uint32_t usec)
{

	mips_timer_udelay(&timer_sc, usec);
}

int
main(void)
{
	uint64_t *addr;
	uint32_t status;

	md_init();

	/* Debug */
	addr = (uint64_t *)(DM_BASE + 0x800000);
	*addr = 0x1515151515161617;

	aju_init(&aju_sc, AJU1_BASE);
	console_register(uart_putchar, (void *)&aju_sc);

	mips_install_intr_map(mips_intr_map);

	beripic_init(&beripic_sc, &beripic1_res);
	beripic_install_intr_map(&beripic_sc, beripic_intr_map);
	beripic_enable(&beripic_sc, 16, 0);

	mips_timer_init(&timer_sc, MIPS_DEFAULT_FREQ);

	status = mips_rd_status();
	status |= MIPS_SR_IM_HARD(0);
	status |= MIPS_SR_IM_HARD(5);
	status |= MIPS_SR_IE;
	status &= ~MIPS_SR_BEV;
	mips_wr_status(status);

	epw_init(&epw_sc, EPW_BASE, EPW_WINDOW);
	epw_control(&epw_sc, 1);

	/* RX FIFO interrupt enable */
	beripic_enable(&beripic_sc, FIFO3_INTR, 0 /* hard IRQ */);

	dm_init(&epw_sc);
	dm_loop(&epw_sc);

	return (0);
}
