/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_DEVICE_INTERFACE_H
#define __ZIPM_DEVICE_INTERFACE_H

/**
 * @brief ZIPM driver generic API definition
 */

typedef int (*zipm_register_event_callback_t)(const struct device *zdev, struct zipm_callback *cs,
				 zipm_event_callback_t fn, void *user_data);
typedef int (*zipm_remove_event_callback_t)(struct zipm_callback *cs);
typedef int (*zipm_send_t)(const struct device *zdev, const void *data, size_t size,
	      int shared_queue_number, int wait_time);
typedef int (*zipm_receive_t)(const struct device *zdev, void *data, size_t *size,
		 int shared_queue_number);
typedef int (*zipm_flush_t)(const struct device *zdev, int shared_queue_number);

struct zipm_device_api {
	zipm_register_event_callback_t register_callback;
	zipm_remove_event_callback_t remove_callback;
	zipm_send_t send;
	zipm_receive_t receive;
	zipm_flush_t flush;
};

#endif
