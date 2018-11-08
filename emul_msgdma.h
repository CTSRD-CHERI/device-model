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

#ifndef	_EMUL_DEVICE_H_
#define	_EMUL_DEVICE_H_

struct msgdma_csr {
	uint32_t dma_status;
	uint32_t dma_control;
};

struct msgdma_pf {
	uint32_t pf_control;
	uint32_t pf_next_lo;
	uint32_t pf_next_hi;
	uint32_t pf_poll_freq;
	uint32_t pf_status;
};

struct msgdma_softc {
	uint32_t state;
	struct msgdma_csr csr;
	struct msgdma_pf pf;
	uint8_t poll_en;
	uint8_t unit;
	struct msgdma_desc *cur_desc;
	struct altera_fifo_softc *fifo_sc;
};

void emul_msgdma(const struct emul_link *elink,
    struct epw_softc *sc, struct epw_request *req);
void emul_msgdma_fifo_intr(void *arg);
void emul_msgdma_poll(struct msgdma_softc *sc);

#endif	/* !_EMUL_DEVICE_H_ */
