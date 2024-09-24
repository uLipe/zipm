/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_H
#define __ZIPM_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/ipm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ZIPM system callback event
 *
 * @param zdev pointer to the ZIPM device
 * @param shared_queue_number desired shared queue to extract data
 * @param user_data pointer where to user defined data
 *
 * @note This callback, once registered, will be called when any of
 * the zipm_event occur, since ZIPM was made to work better in asynch
 * communication, the application can use this callback to be aware of
 * the state of a particular shared queue or memory events. 
 */
typedef void (*zipm_event_callback_t)(const struct device *dev, int shared_queue_number, void *user_data);

struct zipm_callback {
	zipm_event_callback_t callback;
	void *user_data;
	sys_dnode_t link;
};

/*
 * Includes the ZIPM device interface which is used to relay the API below.
 */
#include "zipm_device_interface.h"

/**
 * @brief Register a callback to be called after each zipm event 
 *
 * @param zdev pointer to the ZIPM device
 * @param cs callback structure 
 * @param fn function to be called
 * @param user_data user defined data to pass when callback is called
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int zipm_register_event_callback(const struct device *zdev, struct zipm_callback *cs,
				 zipm_event_callback_t fn, void *user_data)
{
	const struct zipm_device_api *api =
		(const struct zipm_device_api *)zdev->api;

	if (api->register_callback== NULL) {
		return -ENOSYS;
	}

	return api->register_callback(zdev, cs, fn, user_data);
}

/**
 * @brief Removes a previously registered event callback 
 *
 * @param zdev pointer to the ZIPM device
 * @param cs callback structure 
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline  int zipm_remove_event_callback(const struct device *zdev, struct zipm_callback *cs)
{
	const struct zipm_device_api *api =
		(const struct zipm_device_api *)zdev->api;

	if (api->remove_callback== NULL) {
		return -ENOSYS;
	}

	return api->remove_callback(cs);
}

/**
 * @brief Sends data to a shared queue 
 *
 * @param zdev pointer to the ZIPM device
 * @param data data to send
 * @param size size in bytes of the data being sent
 * @param shared_queue_number desired shared queue to put the data
 * @param wait_time time, in miliseconds, to wait for storage to send
 *
 * @return 0 if successful, negative errno code if failure.
 *
 * @note This function automatically fragments the data passed by user
 * if the shared queue block size is smaller than the data size, in that
 * case multiple blocks are but in the queue and marked with NEXT flag
 * the receiver is responsbile on the other hand to extract the data 
 * and reassembly it.
 */
static inline int zipm_send(const struct device *zdev, const void *data, size_t size,
	      int shared_queue_number, int wait_time)
{
	const struct zipm_device_api *api =
		(const struct zipm_device_api *)zdev->api;

	if (api->send== NULL) {
		return -ENOSYS;
	}

	return api->send(zdev, data, size, shared_queue_number, wait_time);
}

/**
 * @brief Receive data from a shared queue 
 *
 * @param zdev pointer to the ZIPM device
 * @param data pointer where to store the data
 * @param size size in bytes of the storage
 * @param shared_queue_number desired shared queue to extract data
 *
 * @return 0 if extracted all the data, positive value if there is
 * more data to extract from the current packet, otherwise negative
 * errno code, -ENOMEM in case of queue empty.
 *
 * @note It is recommended to use this function combined with the event
 * callback and capture the event ZIPM_DATA_PRODUCED, since this function
 * is not synchronous, it will keep returning -ENOMEM immediately if the
 * desired shared queue is empty. The other case is for fragmented data
 * if this function return a positive value, it means the data received
 * was fragmented, so user needs to call this function multiple times
 * until it returns 0 or -ENOMEM to reassembly the fragmented data.
 */
static inline int zipm_receive(const struct device *zdev, void *data, size_t size,
		 int shared_queue_number)
{
	const struct zipm_device_api *api =
		(const struct zipm_device_api *)zdev->api;

	if (api->receive == NULL) {
		return -ENOSYS;
	}

	return api->receive(zdev, data, size, shared_queue_number);
}

/**
 * @brief flushes the shared queue
 *
 * @param zdev pointer to the ZIPM device
 * @param shared_queue_number desired shared queue to flush
 *
 * @return 0 if successful, negative errno code if failure.
 *
 * @note after invoked this function will raise an event of
 * ZIPM_NODE_AVAIL indicating available memory on the shared
 * memory pool.
 */
static inline int zipm_flush(const struct device *zdev, int shared_queue_number)
{
	const struct zipm_device_api *api =
		(const struct zipm_device_api *)zdev->api;

	if (api->flush == NULL) {
		return -ENOSYS;
	}

	return api->flush(zdev, shared_queue_number);
}

#ifdef __cplusplus
}
#endif

#endif
