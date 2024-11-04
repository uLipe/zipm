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
static const char msg[] = {"Hello from other side! Aka secondary core! \0"};
static char rx_msg[64] = {0};
const int doorbell_queue = 0;
const int tx_queue = 1;

static void zipm_event_callback(const struct device *dev, int sq, void *user_data)
{
	if(sq == doorbell_queue) {
		gpio_pin_toggle_dt(&led);
		zipm_receive(dev, &rx_msg, NULL, doorbell_queue);
		zipm_send(dev, &msg, sizeof(msg), tx_queue, 0);

		printk("Primary core asked: %s, so let's say something \n", (const char *)rx_msg);
	}
}

int main(void)
{
	zipm_register_event_callback(handle, &callback, zipm_event_callback, NULL);
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	return 0;
}
