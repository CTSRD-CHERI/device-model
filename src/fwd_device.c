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
#include <sys/systm.h>

#include <mips/beri/beri_epw.h>

#include "device-model.h"
#include "fwd_device.h"

static void
fwd_rw(struct epw_softc *sc, struct epw_request *req,
    uint64_t addr)
{
	void *src, *dst;
	int len;

	if (req->is_write) {
		src = &req->data;
		dst = (void *)addr;
		len = req->byte_enable;
	} else {
		src = (void *)addr;
		dst = &req->data;
		len = req->burst_count;
	}

	switch (len) {
	case 1:
		*(volatile uint8_t *)dst = *(volatile uint8_t *)src;
		break;
	case 2:
		*(volatile uint16_t *)dst = *(volatile uint16_t *)src;
		break;
	case 4:
		*(volatile uint32_t *)dst = *(volatile uint32_t *)src;
		break;
	case 8:
		*(volatile uint64_t *)dst = *(volatile uint64_t *)src;
		break;
	}
}

void
fwd_request(const struct fwd_link *link,
    struct epw_softc *sc, struct epw_request *req)
{
	uint64_t offset;
	uint64_t addr;

	offset = req->addr - link->base_emul - EPW_BASE;
	addr = link->base + offset;

	fwd_rw(sc, req, addr);
}
