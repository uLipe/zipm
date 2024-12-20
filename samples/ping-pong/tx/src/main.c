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
static char rx_msg[64] = {0};
static char tx_msg[] = {"Say something secondary core! \0"};

static int rx_queue = 1;
static int doorbell_queue = 0;

static void zipm_event_callback(const struct device *dev, int sq, void *user_data)
{
	int copied = 0;
	size_t block_size;
	int ret;

	if(sq == rx_queue) {
		gpio_pin_toggle_dt(&led);
		do {
			ret = zipm_receive(handle, &rx_msg[copied], &block_size, rx_queue);
			if(ret > 0) {
				copied += block_size;
			}
		}while(ret != 0);		
		printk("%lld: Other core said: %s \n", k_uptime_get(), (const char *)rx_msg);
	}
}

int main(void)
{
	zipm_register_event_callback(handle, &callback, zipm_event_callback, NULL);

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	
	while(1) {
		k_msleep(500);	
		zipm_send(handle, &tx_msg, sizeof(tx_msg), doorbell_queue, 0);
	}

	return 0;
}
