/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_PLATFORM_BACKEND_H
#define __ZIPM_PLATFORM_BACKEND_H

typedef void (*zipm_platform_doorbell_callback_t)(void struct zipm_platform_backend *zb,
						  void *user_data, void *code);

struct zipm_platform_backend {
	void *platform_handle;
	void *user_data;
	zipm_platform_doorbell_callback_t cb;
};

struct zipm_platform_backend *zipm_platform_backend_get(void *platform_location);
int zipm_platform_backend_send_doorbell(struct zipm_platform_backend *zb, void *code);
int zipm_platform_backend_register_doorbell_callback(struct zipm_platform_backend *zb,
						     zipm_platform_doorbell_callback_t cb,
						     const void *user_data);
int zipm_platform_backend_lock_resource(void *resource);
int zipm_platform_backend_unlock_resource(void *resource);

#endif