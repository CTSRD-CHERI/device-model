/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2012 NetApp, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * Memory ranges are represented with an RB tree. On insertion, the range
 * is checked for overlaps. On lookup, the key has the same base and limit
 * so it can be searched within the range.
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/tree.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem.h"

#define	VM_MAXCPU	1

#define	BHYVE_MEM_DEBUG
#undef	BHYVE_MEM_DEBUG

#ifdef	BHYVE_MEM_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

struct emulate_mem_args {
	int write;
	int access_width;
	uint64_t *val;
};

struct read_mem_args {
	uint64_t *rval;
	int size;
};

struct mmio_rb_range {
	RB_ENTRY(mmio_rb_range)	mr_link;	/* RB tree links */
	struct mem_range	mr_param;
	uint64_t                mr_base;
	uint64_t                mr_end;
};

struct mmio_rb_tree;
RB_PROTOTYPE(mmio_rb_tree, mmio_rb_range, mr_link, mmio_rb_range_compare);

RB_HEAD(mmio_rb_tree, mmio_rb_range) mmio_rb_root, mmio_rb_fallback;

/*
 * Per-vCPU cache. Since most accesses from a vCPU will be to
 * consecutive addresses in a range, it makes sense to cache the
 * result of a lookup.
 */
static struct mmio_rb_range	*mmio_hint[VM_MAXCPU];

void	mmio_rb_dump(struct mmio_rb_tree *rbt);

static int
mmio_rb_range_compare(struct mmio_rb_range *a, struct mmio_rb_range *b)
{

	if (a->mr_end < b->mr_base)
		return (-1);
	else if (a->mr_base > b->mr_end)
		return (1);

	return (0);
}

static int
mmio_rb_lookup(struct mmio_rb_tree *rbt, uint64_t addr,
    struct mmio_rb_range **entry)
{
	struct mmio_rb_range find, *res;

	find.mr_base = find.mr_end = addr;

	res = RB_FIND(mmio_rb_tree, rbt, &find);

	if (res != NULL) {
		*entry = res;
		return (0);
	}
	
	return (ENOENT);
}

static int
mmio_rb_add(struct mmio_rb_tree *rbt, struct mmio_rb_range *new)
{
	struct mmio_rb_range *overlap;

	overlap = RB_INSERT(mmio_rb_tree, rbt, new);

	if (overlap != NULL) {
#ifdef RB_DEBUG
		printf("overlap detected: new %lx:%lx, tree %lx:%lx\n",
		       new->mr_base, new->mr_end,
		       overlap->mr_base, overlap->mr_end);
#endif

		return (EEXIST);
	}

	return (0);
}

void
mmio_rb_dump(struct mmio_rb_tree *rbt)
{
	struct mmio_rb_range *np;

	RB_FOREACH(np, mmio_rb_tree, rbt) {
		printf(" %lx:%lx, %s\n", np->mr_base, np->mr_end,
		       np->mr_param.name);
	}
}

RB_GENERATE(mmio_rb_tree, mmio_rb_range, mr_link, mmio_rb_range_compare);

typedef int (mem_cb_t)(struct vmctx *ctx, int vcpu, uint64_t gpa,
    struct mem_range *mr, void *arg);

static int
access_memory(struct vmctx *ctx, int vcpu, uint64_t paddr, mem_cb_t *cb,
    void *arg)
{
	struct mmio_rb_range *entry;
	int err;
	
	/*
	 * First check the per-vCPU cache
	 */
	if (mmio_hint[vcpu] &&
	    paddr >= mmio_hint[vcpu]->mr_base &&
	    paddr <= mmio_hint[vcpu]->mr_end) {
		entry = mmio_hint[vcpu];
	} else
		entry = NULL;

	if (entry == NULL) {
		if (mmio_rb_lookup(&mmio_rb_root, paddr, &entry) == 0) {
			/* Update the per-vCPU cache */
			mmio_hint[vcpu] = entry;			
		} else if (mmio_rb_lookup(&mmio_rb_fallback, paddr, &entry))
			return (ESRCH);
	}

	assert(entry != NULL);

	err = cb(ctx, vcpu, paddr, &entry->mr_param, arg);

	return (err);
}

static int
emulate_mem_cb(struct vmctx *ctx, int vcpu, uint64_t paddr, struct mem_range *mr,
    void *arg)
{
	struct emulate_mem_args *ema;
	int error;
	int size;

	ema = arg;
	size = ema->access_width;

	dprintf("%s: paddr %lx\n", __func__, paddr);

	if (ema->write == 0)
		error = (*mr->handler)(ctx, vcpu, MEM_F_READ, paddr, size,
		    ema->val, mr->arg1, mr->arg2);
	else
		error = (*mr->handler)(ctx, vcpu, MEM_F_WRITE, paddr, size,
		    ema->val, mr->arg1, mr->arg2);

	return (error);
}

int
emulate_mem(struct vmctx *ctx, int vcpu, uint64_t paddr,
    int write, int access_width, uint64_t *val)
{
	struct emulate_mem_args ema;

	ema.write = write;
	ema.access_width = access_width;
	ema.val = val;
	return (access_memory(ctx, vcpu, paddr, emulate_mem_cb, &ema));
}

static int
read_mem_cb(struct vmctx *ctx, int vcpu, uint64_t paddr, struct mem_range *mr,
    void *arg)
{
	struct read_mem_args *rma;

	rma = arg;
	return (mr->handler(ctx, vcpu, MEM_F_READ, paddr, rma->size,
	    rma->rval, mr->arg1, mr->arg2));
}

int
read_mem(struct vmctx *ctx, int vcpu, uint64_t gpa, uint64_t *rval, int size)
{
	struct read_mem_args rma;

	rma.rval = rval;
	rma.size = size;
	return (access_memory(ctx, vcpu, gpa, read_mem_cb, &rma));
}

static int
register_mem_int(struct mmio_rb_tree *rbt, struct mem_range *memp)
{
	struct mmio_rb_range *entry, *mrp;
	int err;

	err = 0;

	mrp = malloc(sizeof(struct mmio_rb_range));
	if (mrp == NULL) {
		printf("%s: couldn't allocate memory for mrp\n",
		     __func__);
		err = ENOMEM;
	} else {
		mrp->mr_param = *memp;
		mrp->mr_base = memp->base;
		mrp->mr_end = memp->base + memp->size - 1;
		if (mmio_rb_lookup(rbt, memp->base, &entry) != 0)
			err = mmio_rb_add(rbt, mrp);
		if (err)
			free(mrp);
	}

	return (err);
}

int
register_mem(struct mem_range *memp)
{

	printf("%s: name %s base %lx size %lx\n",
	    __func__, memp->name, memp->base, memp->size);

	return (register_mem_int(&mmio_rb_root, memp));
}

int
register_mem_fallback(struct mem_range *memp)
{

	printf("%s: name %s base %lx size %lx\n",
	    __func__, memp->name, memp->base, memp->size);

	return (register_mem_int(&mmio_rb_fallback, memp));
}

int 
unregister_mem(struct mem_range *memp)
{
	struct mem_range *mr;
	struct mmio_rb_range *entry = NULL;
	int err, i;

	dprintf("%s: name %s base %lx size %lx\n",
	    __func__, memp->name, memp->base, memp->size);
	
	err = mmio_rb_lookup(&mmio_rb_root, memp->base, &entry);
	if (err == 0) {
		mr = &entry->mr_param;
		assert(mr->name == memp->name);
		assert(mr->base == memp->base && mr->size == memp->size); 
		assert((mr->flags & MEM_F_IMMUTABLE) == 0);
		RB_REMOVE(mmio_rb_tree, &mmio_rb_root, entry);

		/* flush Per-vCPU cache */	
		for (i=0; i < VM_MAXCPU; i++) {
			if (mmio_hint[i] == entry)
				mmio_hint[i] = NULL;
		}
	}

	if (entry)
		free(entry);
	
	return (err);
}

void
init_mem(void)
{

	RB_INIT(&mmio_rb_root);
	RB_INIT(&mmio_rb_fallback);
}
