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

struct zipm_shared_queue {
    uint32_t magic_1;
    uint32_t write_idx;
    uint32_t read_idx;
    uint32_t avail;
    uint32_t end;
    uint32_t event;
    uint32_t magic_2;
};

int zipm_shared_queue_initialize(void *shared_memory_address, int size);
struct zipm_shared_queue *zipm_shared_queue_access(void *shared_memory_address);
bool zipm_is_shared_queue_valid(const struct zipm_shared_queue *sq);
int zipm_shared_queue_get(struct zipm_shared_queue *sq, struct zipm_node_descriptor *desc);
int zipm_shared_queue_push(struct zipm_shared_queue *sq, const struct zipm_node_descriptor *desc);
int zipm_shared_queue_set_event(struct zipm_shared_queue *sq, uint32_t event);
int zipm_shared_queue_get_event(struct zipm_shared_queue *sq);


#endif