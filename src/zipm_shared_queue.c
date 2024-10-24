/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zipm_shared_queue.h"

#define ZIPM_SHARED_Q_MAGIC_1 	0x5a49504d
#define ZIPM_SHARED_Q_MAGIC_2 	0x51554555

int zipm_shared_queue_initialize(void *shared_memory_address, int size)
{
	struct zipm_shared_queue *sq;

	if(!shared_memory_address)
		return -EINVAL;

	if(!size)
		return - EINVAL;

	sq = (struct zipm_shared_queue *)shared_memory_address;	
	int key = irq_lock();

	sq->event = 0;
	sq->write_idx = 0;
	sq->read_idx = 0;
	sq->avail = 0;
	sq->end = size;
	sq->magic_1 = ZIPM_SHARED_Q_MAGIC_1;
	sq->magic_2 = ZIPM_SHARED_Q_MAGIC_2;

	irq_unlock(key);

	return 0;
}

struct zipm_shared_queue *zipm_shared_queue_access(void *shared_memory_address)
{
	struct zipm_shared_queue *sq;

	if(!shared_memory_address)
		return NULL;

	sq = (struct zipm_shared_queue *)shared_memory_address;
	if(!zipm_is_shared_queue_valid(sq))
		return NULL;

	return(sq);
}

bool zipm_is_shared_queue_valid(const struct zipm_shared_queue *sq)
{
	bool valid = true;

	if(sq == NULL)
		return false;

	if(sq->magic_1 != ZIPM_SHARED_Q_MAGIC_1)
		valid = false;

	if(sq->magic_2 != ZIPM_SHARED_Q_MAGIC_2)
		valid = false;

	return true;
}

int zipm_shared_queue_get(struct zipm_shared_queue *sq, struct zipm_node_descriptor *desc)
{
	struct zipm_node_descriptor *descs;
	int read;
	int end;
	int write;
	int avail;

	if(!sq)
		return -EINVAL;

	int key = irq_lock();

	read = sq->read_idx;
	end = sq->end;
	avail = sq->avail;
	write = sq->write_idx;

	if(!avail) {
		irq_unlock(key);
		return -ENOENT;
	}

	descs = (struct zipm_node_descriptor *)(sq + sizeof(struct zipm_shared_queue));
	desc->addr = descs[read].addr;
	desc->flags = descs[read].flags;
	desc->size = descs[read].size;
	
	read = ((read + 1) % end);
	if(avail > 0)
		avail--;

	atomic_set((atomic_t *)&sq->read_idx, read);	
	atomic_set((atomic_t *)&sq->avail, avail);	

	irq_unlock(key);

	return 0;
}

int zipm_shared_queue_push(struct zipm_shared_queue *sq, const struct zipm_node_descriptor *desc)
{
	struct zipm_node_descriptor *descs;
	int read;
	int write;
	int end;
	int avail;

	if(!sq || !desc)
		return -EINVAL;

	int key = irq_lock();

	write = sq->write_idx;
	read = sq->read_idx;
	end = sq->end;
	avail = sq->avail;

	if(avail == end) {
		irq_unlock(key);
		return -ENOENT;
	}

	descs = (struct zipm_node_descriptor *)(sq + sizeof(struct zipm_shared_queue));
	descs[write].addr = desc->addr;
	descs[write].flags = desc->flags;
	descs[write].size = desc->size;

	write = ((write + 1) % end);
	if(avail < end)
		avail++;

	atomic_set((atomic_t *)&sq->write_idx, write);
	atomic_set((atomic_t *)&sq->avail, avail);
	irq_unlock(key);

	return 0;
}

int zipm_shared_queue_set_event(struct zipm_shared_queue *sq, uint32_t event)
{
	if(!sq)
		return -EINVAL;

	int key = irq_lock();
	atomic_set((atomic_t *)&sq->event, event);
	irq_unlock(key);

	return 0;
}

int zipm_shared_queue_get_event(struct zipm_shared_queue *sq) 
{
	uint32_t ev;

	if(!sq)
		return -EINVAL;

	int key = irq_lock();
	ev = sq->event;
	irq_unlock(key);

	return ev;
}