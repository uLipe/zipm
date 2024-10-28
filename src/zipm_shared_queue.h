/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_SHARED_QUEUE_H
#define __ZIPM_SHARED_QUEUE_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include "zipm_node.h"

__packed struct zipm_shared_queue {
    uint32_t magic_1;
    uint32_t write_idx;
    uint32_t read_idx;
    uint32_t avail;
    uint32_t end;
    uint32_t magic_2;
    struct zipm_node_descriptor *descs;
};

int zipm_shared_queue_initialize(volatile void *shared_memory_address, int size);
struct zipm_shared_queue *zipm_shared_queue_access(volatile void *shared_memory_address);

bool zipm_is_shared_queue_valid(volatile const struct zipm_shared_queue *sq);

int zipm_shared_queue_get(volatile struct zipm_shared_queue *sq, struct zipm_node_descriptor *desc);
int zipm_shared_queue_push(volatile struct zipm_shared_queue *sq, const struct zipm_node_descriptor *desc);

bool zipm_shared_queue_has_data(volatile struct zipm_shared_queue *sq);

#endif