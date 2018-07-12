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

#include <mips/beri/beri_epw.h>

#include "device-model.h"
#include "fwd_device.h"

#define DM_NDEVICES	2

const struct device_link device_map[DM_NDEVICES] = {
	{ 0x0000, 0x40, MSGDMA0_BASE, fwd_request },
	{ 0x0040, 0x40, MSGDMA1_BASE, fwd_request },
};

static void
dm_request(struct epw_softc *sc, struct epw_request *req)
{
	const struct device_link *link;
	uint64_t offset;
	int i;

	offset = req->addr - EPW_BASE;

	for (i = 0; i < DM_NDEVICES; i++) {
		link = &device_map[i];
		if (offset >= link->base_emul &&
		    offset < (link->base_emul + link->size))
			link->request(link, sc, req);
	}
}

void
dm_loop(struct epw_softc *sc)
{
	struct epw_request req;

	while (1) {
		printf("Hello World!\n");

		if (epw_request(sc, &req) != 0) {
			dm_request(sc, &req);
			epw_reply(sc, &req);
		}

		usleep(1000000);
	}
}
