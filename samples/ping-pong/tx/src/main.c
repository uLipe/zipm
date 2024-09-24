/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zipm/zipm.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *handle = DEVICE_DT_GET(DT_NODELABEL(zipm_device0));
static struct zipm_callback callback;
static const char rx_msg[64] = {0};

static int rx_queue = 1;
static int doorbell_queue = 0;

int main(void)
{
	int i = 0;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return 0;
	}

	while(1) {
		k_msleep(500);
		printk("Sending doorbel %d ! \n", i++);
		zipm_send(handle, NULL, 0, doorbell_queue, 0);
	}

	return 0;
}
