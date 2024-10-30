/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_NODE_POOL_H
#define __ZIPM_NODE_POOL_H

#include <stdbool.h>
#include <stdint.h>
#include "zipm_core_portable.h"
#include "zipm_node.h"

__packed struct zipm_node_pool_header {
    uint32_t magic_1;
    unsigned long control;
    uint32_t blocks_avail;
    uint32_t block_size;
    sys_dlist_t descriptors;
    uint32_t magic_2;
};

int zipm_node_pool_initialize(uint32_t block_size, uint32_t blocks_avail,
			      volatile void *shared_memory_address);
struct zipm_node_pool_header *zipm_node_pool_access(volatile void *shared_memory_address);

bool zipm_node_pool_is_valid(volatile const struct zipm_node_pool_header *h);

uint8_t *zipm_node_pool_alloc(volatile struct zipm_node_pool_header *h);
int zipm_node_pool_dealloc(volatile struct zipm_node_pool_header *h, uint8_t *desc);

#endif
