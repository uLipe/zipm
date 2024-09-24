/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zipm_shared_queue.h"

int zipm_shared_queue_initialize(void *shared_memory_address)
{
	return 0;
}

struct zipm_shared_queue *zipm_shared_queue_access(void *shared_memory_address)
{
	return NULL;
}

bool zipm_is_shared_queue_valid(const struct zipm_shared_queue *sq)
{
	return true;
}

struct zipm_node_descriptor *zipm_shared_queue_peek(const struct zipm_shared_queue *sq)
{
	struct zipm_node_descriptor *node = NULL;

	return node;
}

struct zipm_node_descriptor *zipm_shared_queue_get(const struct zipm_shared_queue *sq)
{
	struct zipm_node_descriptor *node = NULL;

	return node;
}


int zipm_shared_queue_push(struct zipm_shared_queue *sq, const struct zipm_node_descriptor *elem)
{
	return 0;
}

int zipm_shared_queue_set_event(struct zipm_shared_queue *sq, uint32_t event)
{
	return 0;
}
