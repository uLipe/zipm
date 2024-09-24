/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zipm/zipm.h>
#include <zipm/zipm_backend.h>
#include "zipm_node_pool.h"
#include "zipm_shared_queue.h"

int zipm_initialise_handle(struct zipm_handle *handle, uintptr_t node_pool_area,
			   uintptr_t shared_queue_area)
{
	return 0;
}

int zipm_register_event_callback(const struct zipm_handle *handle, struct zipm_callback *cs,
				 zipm_event_callback_t fn, void *user_data)
{
	return 0;
}

int zipm_send(const struct zipm_handle *handle, const void *data, size_t size,
	      int wait_time)
{
	return 0;
}

int zipm_receive(const struct zipm_handle *handle, void *data, size_t *received_size,
		 int wait_time)
{
	return 0;
}

int zipm_send_doorbell(const struct zipm_handle *handle)
{
	return 0;
}

int zipm_wait_doorbell(const struct zipm_handle *handle, int wait_time)
{
	return 0;
}

int zipm_flush(const struct zipm_handle *handle)
{
	return 0;
}

int zipm_release_handle(const struct zipm_handle *handle)
{
	return 0;
}
