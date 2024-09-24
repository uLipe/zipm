/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_H
#define __ZIPM_H

#include <sdtbool.h>
#include <stdint.h>
#include <util/util.h>
#include <util/ul_list.h>
#include "zipm_platform_backend.h"

enum zipm_event {
	ZIPM_TX_COMPLETE = 0,
	ZIPM_RX_COMPLETE,
	ZIPM_DOORBELL,
	ZIPM_TX_QUEUE_EMPTY,
	ZIPM_NODE_POOL_EMPTY,
};

typedef void (*zipm_event_callback_t)(const struct zipm_handle *handle, enum zipm_event e);

struct zipm_callback {
	zipm_event_callback_t callback;
	void *user_data;
	ul_dnode_t link;
}

struct zipm_handle {
	struct zipm_platform_backend *backend;
	uintptr_t node_pool_location;
	uintptr_t shared_queue_location;
	struct ul_dlist callbacks;
};

int zipm_initialise_handle(struct zipm_handle *handle, uintptr_t node_pool_area,
			   uintptr_t shared_queue_area, void *platform_backend);
int zipm_register_event_callback(const struct zipm_handle *handle, struct zipm_callback *cs,
				 zipm_event_callback_t fn, void *user_data);
int zipm_send(const struct zipm_handle *handle, const void *data, size_t size,
	      int wait_time);
int zipm_receive(const struct zipm_handle *handle, void *data, size_t *received_size,
		 int wait_time);
int zipm_send_doorbell(const struct zipm_handle *handle);
int zipm_wait_doorbell(const struct zipm_handle *handle, int wait_time);
int zipm_flush(const struct zipm_handle *handle);
int zipm_release_handle(const struct zipm_handle *handle);

#endif