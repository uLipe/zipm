/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_NODE_H
#define __ZIPM_NODE_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#define ZIPM_NODE_FLAGS_END     (1 << 0)
#define ZIPM_NODE_FLAGS_NEXT    (1 << 1)

struct zipm_node_descriptor {
    uint32_t magic_1;
    uint32_t magic_2;
    uint32_t flags;
    sys_dnode_t link;
    uint8_t memory[1];
};

#endif
