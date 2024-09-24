/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "zipm_node_pool.h"

int zipm_node_pool_initialize(struct zipm_node_pool_header *h, uint32_t block_size,
			      uint32_t blocks_avail, void *shared_memory_address)
{
    return 0;
}

bool zipm_node_pool_is_valid(const struct zipm_node_pool_header *h)
{
    return true;
}

struct zipm_node_descriptor *zipm_node_pool_alloc(const struct zipm_node_pool_header *h)
{
    struct zipm_node_descriptor *node;

    return node;
}

int zipm_node_pool_dealloc(const struct zipm_node_pool_header *h, struct zipm_node_descriptor *desc)
{
    return 0;
}

