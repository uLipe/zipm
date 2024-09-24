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

void zipm_event_callback(const struct device *dev, int shared_queue_number, void *user_data)
{
}

static void zipm_tests_before(void *f)
{
}

static void zipm_tests_after(void *f)
{
}

ZTEST(zipm_test_suite, zipm_initialise_handler)
{
	const struct device *handle = DEVICE_DT_GET(DT_NODELABEL(zipm_device0));
	struct zipm_callback callback; 

	zassert_equal(zipm_register_event_callback(handle, &callback,
				 zipm_event_callback, NULL), 0);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);
	zassert_equal(zipm_remove_event_callback(handle,&callback), 0);
}

ZTEST(zipm_test_suite, send_receive)
{
	uint8_t data[] = {'Z','I','P','M'};
	uint8_t rx_data[] = {0,0,0,0};
	size_t rx_size = 0;

	const struct device *handle = DEVICE_DT_GET(DT_NODELABEL(zipm_device0));
	struct zipm_callback callback; 

	zassert_equal(zipm_register_event_callback(handle, &callback,
				 zipm_event_callback, NULL), 0);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);

	zassert_equal(zipm_send(handle, &data, sizeof(data), 0, 0), 0);
	zassert_equal(zipm_receive(handle, &rx_data, rx_size, 0), 0);
	zassert_equal(rx_size, sizeof(data));
	zassert_equal(memcmp(data, rx_data, sizeof(rx_data)), 0);
	zassert_equal(zipm_flush(handle, 0), 0);
	zassert_equal(zipm_remove_event_callback(handle,&callback), 0);
}

ZTEST(zipm_test_suite, send_exaust_memory)
{
	uint8_t data[] = {'Z','I','P','M'};
	uint8_t rx_data[] = {0,0,0,0};
	size_t rx_size = 0;
	int err = 0;

	const struct device *handle = DEVICE_DT_GET(DT_NODELABEL(zipm_device0));
	struct zipm_callback callback; 

	zassert_equal(zipm_register_event_callback(handle, &callback,
				 zipm_event_callback, NULL), 0);
	zassert_equal(callback.callback, &zipm_event_callback);
	zassert_equal(callback.user_data, &callback);

	do {
		err = zipm_send(handle, &data, sizeof(data), 0, 0);
	} while(!err);

	err = 0;

	do {
		err = zipm_receive(handle, &rx_data, rx_size, 0);
		if(!err) {
			zassert_equal(rx_size, sizeof(data));
			zassert_equal(memcmp(data, rx_data, sizeof(rx_data)), 0);
		}
	} while(!err);

	zassert_equal(zipm_flush(handle, 0), 0);
	zassert_equal(zipm_remove_event_callback(handle,&callback), 0);
}

ZTEST_SUITE(zipm_test_suite, NULL, NULL, zipm_tests_before, zipm_tests_after, NULL);
