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
#include <sys/malloc.h>
#include <sys/cheri.h>

#include <machine/frame.h>
#include <machine/cpuregs.h>
#include <machine/cpufunc.h>
#include <machine/cheric.h>

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

void * __capability kernel_sealcap;

struct beripic_resource beripic0_res;
struct beripic_resource beripic1_res;

static struct beripic_softc beripic0_sc = { .res = &beripic0_res };
static struct beripic_softc beripic1_sc = { .res = &beripic1_res };

struct mdx_device beripic0 = { .sc = &beripic0_sc };
struct mdx_device beripic1 = { .sc = &beripic1_sc };

static struct aju_softc aju_sc;
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

void
board_init(void)
{
	void *malloc_base;
	int malloc_size;
	uint32_t status;
	capability cap;
	capability base, window;
	int error;

#if 0
	uint64_t *addr;
	/* Debug */
	addr = (uint64_t *)(DM_BASE + 0x800000);
	*addr = 0x1515151515161617;
#endif

	cap = cheri_setoffset(cheri_getdefault(),
	    MIPS_XKPHYS_UNCACHED_BASE + AJU1_BASE);
	cap = cheri_csetbounds(cap, 8);

	aju_init(&aju_sc, cap);
	mdx_console_register(uart_putchar, (void *)&aju_sc);

	CHERI_PRINT_PTR(cap);

	mips_setup_intr(0, softintr, NULL);
	mips_setup_intr(1, softintr, NULL);
	mips_setup_intr(2, beripic_intr, (void *)&beripic1);
	mips_setup_intr(3, hardintr, NULL);
	mips_setup_intr(4, hardintr, NULL);
	mips_setup_intr(5, hardintr, NULL);
	mips_setup_intr(6, hardintr, NULL);
	mips_setup_intr(7, mips_timer_intr, (void *)&timer_sc);

	cap = cheri_getdefault();

	/* The beripic of this CPU core. */
	beripic1_res.cfg = cheri_setoffset(cap,
	    BERIPIC1_CFG | MIPS_XKPHYS_UNCACHED_BASE);
	beripic1_res.ip_read = cheri_setoffset(cap,
	    BERIPIC1_IP_READ | MIPS_XKPHYS_UNCACHED_BASE);
	beripic1_res.ip_set = cheri_setoffset(cap,
	    BERIPIC1_IP_SET | MIPS_XKPHYS_UNCACHED_BASE);
	beripic1_res.ip_clear = cheri_setoffset(cap,
	    BERIPIC1_IP_CLEAR | MIPS_XKPHYS_UNCACHED_BASE);
	beripic_init(&beripic1, &beripic1_res);
	beripic_install_intr_map(&beripic1, beripic_intr_map);

	/*
	 * The beripic of the main core (FreeBSD).
	 * It does not require beripic_init().
	 */
	beripic0_res.cfg = cheri_setoffset(cap,
	    BERIPIC0_CFG | MIPS_XKPHYS_UNCACHED_BASE);
	beripic0_res.ip_read = cheri_setoffset(cap,
	    BERIPIC0_IP_READ | MIPS_XKPHYS_UNCACHED_BASE);
	beripic0_res.ip_set = cheri_setoffset(cap,
	    BERIPIC0_IP_SET | MIPS_XKPHYS_UNCACHED_BASE);
	beripic0_res.ip_clear = cheri_setoffset(cap,
	    BERIPIC0_IP_CLEAR | MIPS_XKPHYS_UNCACHED_BASE);

	/* Enable IPI from FreeBSD. */
	beripic_enable(&beripic1, 16, 0);

	error = mips_timer_init(&timer_sc, MIPS_DEFAULT_FREQ);
	if (error)
		panic("could not initialize mips timer, error %d\n", error);

	status = mips_rd_status();
	status |= MIPS_SR_IM_HARD(0);
	status |= MIPS_SR_IM_HARD(5);
	status |= MIPS_SR_IE;
	status &= ~MIPS_SR_BEV;
	status |= MIPS_SR_UX;
	status |= MIPS_SR_KX;
	status |= MIPS_SR_SX;
	mips_wr_status(status);

	/* Select 4K page for TLB. */
	mtc0(5, 0, 0);

	base = cheri_setoffset(cheri_getdefault(),
	    EPW_BASE | MIPS_XKPHYS_UNCACHED_BASE);
	window = cheri_setoffset(cap, EPW_WINDOW | MIPS_XKPHYS_UNCACHED_BASE);
	epw_init(&epw_sc, base, window);

	/* Enable EPW */
	epw_control(&epw_sc, 1);

	/* Enable RX FIFO interrupt. */
	beripic_enable(&beripic1, FIFO3_INTR, 0 /* hard IRQ */);

	printf("%s: Initializing malloc\n", __func__);
	malloc_init();
#ifdef __CHERI_PURE_CAPABILITY__
	malloc_base = cheri_setoffset(cheri_getdefault(),
	    DM_BASE + 0x01000000 / 2);
#else
	malloc_base = (void *)(DM_BASE + 0x01000000/2);
#endif
	malloc_size = 0x01000000/2;
	malloc_add_region((void *)malloc_base, malloc_size);
}

#if 1
void tlb_nosegtab0(void);
void tlb_nosegtab1(void);
void tlb_nosegtab2(void);
void tlb_nosegtab3(void);
void tlb_miss(void);

void
tlb_nosegtab0(void)
{

	printf("%s: badvaddr %lx\n", __func__, mfc0(8, 0));
	while (1);
}

void
tlb_nosegtab1(void)
{

	printf("%s: badvaddr %lx\n", __func__, mfc0(8, 0));
	while (1);
}

void
tlb_nosegtab2(void)
{

	printf("%s: badvaddr %lx\n", __func__, mfc0(8, 0));
	while (1);
}

void
tlb_nosegtab3(void)
{

	printf("%s: badvaddr %lx\n", __func__, mfc0(8, 0));
	while (1);
}

void
tlb_miss(void)
{

	printf("tlb miss\n");
}
#endif

int
main(void)
{

	printf("%s\n", __func__);

	dm_init(&epw_sc);
	dm_loop(&epw_sc);

	panic("dm_loop returned");

	return (0);
}
