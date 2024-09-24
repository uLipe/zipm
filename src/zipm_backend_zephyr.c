/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zipm/zipm_pratform_backend.h>

struct zipm_platform_backend *zipm_platform_backend_get(void *platform_location)
{
	return NULL;
}

int zipm_platform_backend_send_doorbell(struct zipm_platform_backend *zb, void *code)
{
	return 0;
}

int zipm_platform_backend_register_doorbell_callback(struct zipm_platform_backend *zb,
						     zipm_platform_doorbell_callback_t cb,
						     const void *user_data)
{
	return 0;
}

int zipm_platform_backend_lock_resource(void *resource)
{
	return 0;
}

int zipm_platform_backend_unlock_resource(void *resource)
{
	return 0;
}

