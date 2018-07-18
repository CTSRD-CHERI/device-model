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
#include <dev/altera/msgdma/msgdma.h>

#include "device-model.h"
#include "emul_msgdma.h"

static void
csr_r(uint64_t offset, struct epw_request *req)
{

	switch (offset) {
	case DMA_STATUS:
		break;
	case DMA_CONTROL:
		break;
	};
}

static void
csr_w(uint64_t offset, struct epw_request *req)
{

	switch (offset) {
	case DMA_STATUS:
		break;
	case DMA_CONTROL:
		break;
	};
}

static void
pf_r(uint64_t offset, struct epw_request *req)
{

	switch (offset) {
	case PF_CONTROL:
	case PF_NEXT_LO:
	case PF_NEXT_HI:
	case PF_POLL_FREQ:
	case PF_STATUS:
		break;
	};
}

static void
pf_w(uint64_t offset, struct epw_request *req)
{

	switch (offset) {
	case PF_CONTROL:
	case PF_NEXT_LO:
	case PF_NEXT_HI:
	case PF_POLL_FREQ:
	case PF_STATUS:
		break;
	};
}

void
emul_msgdma(const struct emul_link *elink, struct epw_softc *sc,
    struct epw_request *req)
{
	uint64_t offset;

	offset = req->addr - elink->base_emul - EPW_BASE;

	if (elink->type == MSGDMA_CSR)
		if (req->is_write)
			csr_w(offset, req);
		else
			csr_r(offset, req);
	else
		if (req->is_write)
			pf_w(offset, req);
		else
			pf_r(offset, req);
}
