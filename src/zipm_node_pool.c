/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include "zipm_node_pool.h"
#include <zephyr/sys/atomic.h>

#define ZIPM_NODE_POOL_MAGIC_1		0x5a49504d
#define ZIPM_NODE_POOL_MAGIC_2 		0x4e4f4445
#define ZIPM_NODE_POOL_LOCK_FREE	0x4d50495a
#define ZIPM_NODE_POOL_MIN_BSIZE	16

static inline void zipm_node_pool_lock(volatile const struct zipm_node_pool_header *h)
{
	int key = irq_lock();
	while (!atomic_cas((atomic_val_t *)&h->control, ZIPM_NODE_POOL_LOCK_FREE,
			   ZIPM_NODE_POOL_MAGIC_1)) {
		;
	}
	irq_unlock(key);
}

static inline void zipm_node_pool_unlock(volatile const struct zipm_node_pool_header *h)
{
	int key = irq_lock();
	atomic_set((atomic_val_t *)&h->control, ZIPM_NODE_POOL_LOCK_FREE);
	irq_unlock(key);

}

int zipm_node_pool_initialize(uint32_t block_size,
			      uint32_t blocks_avail, volatile void *shared_memory_address)
{
	struct zipm_node_pool_header *h;
	sys_dnode_t *d;
	uint32_t addr = (uint32_t)shared_memory_address;

	if(shared_memory_address == NULL)
		return -EINVAL;

	if(!block_size || !blocks_avail)
		return -EINVAL;

	if(block_size < ZIPM_NODE_POOL_MIN_BSIZE) {
		return -EINVAL;
	}

	h = (struct zipm_node_pool_header *)shared_memory_address;
	addr += (sizeof(struct zipm_node_pool_header));
	d = (sys_dnode_t *)addr;

	int key = irq_lock();
	atomic_set((atomic_t *)&h->control, 0);
	h->block_size = block_size;
	h->blocks_avail = blocks_avail;
	sys_dlist_init(&h->descriptors);

	for(int i = 0; i < h->blocks_avail; i++) {
		sys_dlist_append(&h->descriptors, d);
		addr += h->block_size;
		d = (sys_dnode_t *)addr;
	}

	h->magic_1 = ZIPM_NODE_POOL_MAGIC_1;
	h->magic_2 = ZIPM_NODE_POOL_MAGIC_2;
	atomic_set((atomic_t *)&h->control, ZIPM_NODE_POOL_LOCK_FREE);
	irq_unlock(key);

	return 0;
}

struct zipm_node_pool_header *zipm_node_pool_access(volatile void *shared_memory_address)
{
	struct zipm_node_pool_header *h = (struct zipm_node_pool_header *)shared_memory_address;

	if(shared_memory_address == NULL)
		return NULL;

	if(!zipm_node_pool_is_valid(h))
		return NULL;

	return h;
}

bool zipm_node_pool_is_valid(volatile const struct zipm_node_pool_header *h)
{
	bool valid = true;
	if(h == NULL)
		return false;

	if(h->magic_1 != ZIPM_NODE_POOL_MAGIC_1)
		valid = false;

	if(h->magic_2 != ZIPM_NODE_POOL_MAGIC_2)
		valid = false;

	return valid;
}

uint8_t *zipm_node_pool_alloc(volatile struct zipm_node_pool_header *h)
{
	sys_dnode_t *node;

	if(h == NULL)
		return NULL;

	int key = irq_lock();
	zipm_node_pool_lock(h);

	node = sys_dlist_peek_head((sys_dlist_t *)&h->descriptors);
	if(!node) {
		zipm_node_pool_unlock(h);
		irq_unlock(key);
		return NULL;
	}

	sys_dlist_remove(node);
	zipm_node_pool_unlock(h);
	irq_unlock(key);

	return (uint8_t *)node;
}

int zipm_node_pool_dealloc(volatile struct zipm_node_pool_header *h, uint8_t *desc)
{
	sys_dnode_t *node;

	if(!h || !desc)
		return -EINVAL;

	int key = irq_lock();
	
	node = (sys_dnode_t *)desc;
	zipm_node_pool_lock(h);
	sys_dlist_append((sys_dlist_t *)&h->descriptors, node);
	zipm_node_pool_unlock(h);

	irq_unlock(key);

	return 0;
}

