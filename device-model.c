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
#include <sys/console.h>
#include <sys/endian.h>
#include <sys/systm.h>

#include <mips/mips/timer.h>
#include <mips/beri/beripic.h>
#include <dev/altera/jtag_uart.h>

#include <machine/cpufunc.h>
#include <machine/cpuregs.h>

#include "device-model.h"

struct beripic_resource beripic1_res = {
	.cfg = BERIPIC1_CFG,
	.ip_read = BERIPIC1_IP_READ,
	.ip_set = BERIPIC1_IP_SET,
	.ip_clear = BERIPIC1_IP_CLEAR,
};

static struct aju_softc aju_sc;
static struct beripic_softc beripic_sc;
static struct mips_timer_softc timer_sc;

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

	/* Debug */
	addr = (uint64_t *)0xffffffffb0800000;
	*addr = 0x1515151515161617;

	aju_init(&aju_sc, AJU1_BASE);

	console_register(uart_putchar, (void *)&aju_sc);

	beripic_init(&beripic_sc, &beripic1_res);
	beripic_enable(&beripic_sc, 16, 0);

	status = mips_rd_status();
	status |= MIPS_SR_IM_HARD(0);
	status |= MIPS_SR_IE;
	status &= ~MIPS_SR_BEV;
	mips_wr_status(status);

	mips_timer_init(&timer_sc, MIPS_DEFAULT_FREQ);

	while (1) {
		printf("Hello World!\n");
		udelay(1000000);
	}

	return (0);
}

void
exception(void)
{

	printf("Exception!\n");
}
