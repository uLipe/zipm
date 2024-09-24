/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_NODE_POOL_H
#define __ZIPM_NODE_POOL_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include "zipm_node.h"

struct zipm_node_pool_header {
    uint32_t magic_1;
    uint32_t magic_2;
    uint32_t control;
    uint32_t block_size;
    uint32_t blocks_avail;
    sys_dlist_t descriptors;
};

int zipm_node_pool_initialize(struct zipm_node_pool_header *h, uint32_t block_size,
			      uint32_t blocks_avail, void *shared_memory_address);

bool zipm_node_pool_is_valid(const struct zipm_node_pool_header *h);

struct zipm_node_descriptor *zipm_node_pool_alloc(const struct zipm_node_pool_header *h);
int zipm_node_pool_dealloc(const struct zipm_node_pool_header *h, struct zipm_node_descriptor *desc);

#endif
