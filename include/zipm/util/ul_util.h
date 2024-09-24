/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __UL_UTIL_H
#define __UL_UTIL_H

#include <stddef.h>

#define UL_CONTAINER_OF(ptr, type, field)  ((type *)(((char *)(ptr)) - offsetof(type, field)))

#endif