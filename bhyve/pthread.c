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
#include <assert.h>

#include "pthread.h"

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{

	assert(mutex->lock == 0);

	mutex->lock = 1;

	return (0);
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{

	assert(mutex->lock == 1);

	mutex->lock = 0;

	return (0);
}

int
pthread_mutex_isowned_np(pthread_mutex_t *mutex)
{

	return (mutex->lock);
}

int
pthread_mutex_init(pthread_mutex_t *restrict mutex,
    const pthread_mutexattr_t *restrict attr)
{

	mutex->lock = 0;

	return (0);
}

int
pthread_cond_signal(pthread_cond_t *cond)
{

	return (0);
}

int
pthread_cond_init(pthread_cond_t *restrict cond,
    const pthread_condattr_t *restrict attr)
{

	return (0);
}

void
pthread_set_name_np(pthread_t thread, const char *name)
{

}

int
pthread_cond_wait(pthread_cond_t *restrict cond,
    pthread_mutex_t *restrict mutex)
{

	return (0);
}

int
pthread_create(pthread_t *restrict thread,
    const pthread_attr_t *restrict attr, void *(*start_routine)(void *),
    void *restrict arg)
{

	return (0);
}
