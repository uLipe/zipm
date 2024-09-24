/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zipm/zipm.h>
#include <zephyr/ztest.h>


void zipm_event_callback(const struct zipm_handle *handle, enum zipm_event e)
{
	return 0;
}

static void zipm_tests_before(void *f)
{
}

static void zipm_tests_after(void *f)
{
}

ZTEST(zipm_test_suite, zipm_initialise_handler)
{
	struct zipm_handle handle;
	struct zipm_callback callback; 
	const struct device *ipc = DT_CHOSEN(zephyr_ipc);

	zassert_equal(zipm_initialise_handle(&handle, DT_REG_ADDRESS(zipm_pool),
			   DT_REG_ADDRESS(zipm_tx_queue), &ipc), 0);
	zassert_equal(handle.node_pool_location, DT_REG_ADDRESS(zipm_pool));
	zassert_equal(handle.shared_queue_location, DT_REG_ADDRESS(zipm_tx_queue))
	zassert_not_equal(handle.backend, NULL);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), true);

	zassert_equal(zipm_register_event_callback(&handle, &callback,
				 zipm_event_callback, &callback), 0);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), false);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);
	zassert_equal(zipm_flush(&handle), 0);
	zassert_equal(zipm_release_handle(&handle), 0);
}

ZTEST(zipm_test_suite, zipm_initialise_handler_not_alloc)
{
	struct zipm_handle handle;
	struct zipm_callback callback; 
	const struct device *ipc = DT_CHOSEN(zephyr_ipc);

	zassert_equal(zipm_initialise_handle(&handle, NULL,
			   DT_REG_ADDRESS(zipm_tx_queue), &ipc), 0);
	zassert_equal(handle.node_pool_location, NULL);
	zassert_equal(handle.shared_queue_location, DT_REG_ADDRESS(zipm_tx_queue))
	zassert_not_equal(handle.backend, NULL);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), true);

	zassert_equal(zipm_register_event_callback(&handle, &callback,
				 zipm_event_callback, &callback), 0);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), false);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);
	zassert_equal(zipm_flush(&handle), 0);
	zassert_equal(zipm_release_handle(&handle), 0);
}

ZTEST(zipm_test_suite, send_receive)
{
	uint8_t data[] = {'Z','I','P','M'};
	uint8_t rx_data[] = {0,0,0,0};
	size_t rx_size = 0;

	struct zipm_handle handle;
	struct zipm_callback callback; 
	const struct device *ipc = DT_CHOSEN(zephyr_ipc);

	zassert_equal(zipm_initialise_handle(&handle, DT_REG_ADDRESS(zipm_pool),
			   DT_REG_ADDRESS(zipm_tx_queue), &ipc), 0);
	zassert_equal(handle.node_pool_location, DT_REG_ADDRESS(zipm_pool));
	zassert_equal(handle.shared_queue_location, DT_REG_ADDRESS(zipm_tx_queue))
	zassert_not_equal(handle.backend, NULL);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), true);

	zassert_equal(zipm_register_event_callback(&handle, &callback,
				 zipm_event_callback, &callback), 0);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), false);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);

	zassert_equal(zipm_send(&handle, &data, sizeof(data), 0), 0);
	zassert_equal(zipm_receive(&handle, &rx_data, &rx_size, 0), 0);
	zassert_equal(rx_size, sizeof(data));
	zassert_equal(memcmp(data, rx_data, sizeof(rx_data)), 0);
	zassert_equal(zipm_flush(&handle), 0);
	zassert_equal(zipm_release_handle(&handle), 0);
}

ZTEST(zipm_test_suite, send_no_memory)
{
	uint8_t data[] = {'Z','I','P','M'};
	uint8_t rx_data[] = {0,0,0,0};
	size_t rx_size = 0;

	struct zipm_handle handle;
	struct zipm_callback callback; 
	const struct device *ipc = DT_CHOSEN(zephyr_ipc);

	zassert_equal(zipm_initialise_handle(&handle, NULL,
			   DT_REG_ADDRESS(zipm_tx_queue), &ipc), 0);
	zassert_equal(handle.node_pool_location, DT_REG_ADDRESS(zipm_pool));
	zassert_equal(handle.shared_queue_location, DT_REG_ADDRESS(zipm_tx_queue))
	zassert_not_equal(handle.backend, NULL);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), true);

	zassert_equal(zipm_register_event_callback(&handle, &callback,
				 zipm_event_callback, &callback), 0);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), false);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);

	zassert_equal(zipm_send(&handle, &data, sizeof(data), 0), -ENOMEM);
	zassert_equal(zipm_flush(&handle), 0);
	zassert_equal(zipm_release_handle(&handle), 0);
}

ZTEST(zipm_test_suite, send_exaust_memory)
{
	uint8_t data[] = {'Z','I','P','M'};
	uint8_t rx_data[] = {0,0,0,0};
	size_t rx_size = 0;
	int err = 0;

	struct zipm_handle handle;
	struct zipm_callback callback; 
	const struct device *ipc = DT_CHOSEN(zephyr_ipc);

	zassert_equal(zipm_initialise_handle(&handle, DT_REG_ADDRESS(zipm_pool),
			   DT_REG_ADDRESS(zipm_tx_queue), &ipc), 0);
	zassert_equal(handle.node_pool_location, DT_REG_ADDRESS(zipm_pool));
	zassert_equal(handle.shared_queue_location, DT_REG_ADDRESS(zipm_tx_queue))
	zassert_not_equal(handle.backend, NULL);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), true);

	zassert_equal(zipm_register_event_callback(&handle, &callback,
				 zipm_event_callback, &callback), 0);
	zassert_equal(ul_dlist_is_empty(&handle.callbacks), false);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);

	do {
		err = zipm_send(&handle, &data, sizeof(data), 0);
	} while(!err);

	err = 0;

	do {
		err = zipm_receive(&handle, &rx_data, &rx_size, 0);
		if(!err) {
			zassert_equal(rx_size, sizeof(data));
			zassert_equal(memcmp(data, rx_data, sizeof(rx_data)), 0);
		}
	} while(!err);

	zassert_equal(zipm_flush(&handle), 0);
	zassert_equal(zipm_release_handle(&handle), 0);
}

ZTEST_SUITE(zipm_test_suite, NULL, NULL, zipm_tests_before, zipm_tests_after, NULL);
