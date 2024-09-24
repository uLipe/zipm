/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zipm_message_device

#include <errno.h>
#include <zipm/zipm.h>
#include "zipm_node_pool.h"
#include "zipm_shared_queue.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zipm, CONFIG_LOG_DEFAULT_LEVEL);

struct zipm_device_config {
	const struct device *ipc_device;
	struct zipm_node_pool_header* node_pool;
	uint32_t node_pool_location;
	uint32_t node_pool_block_size;
    uint32_t node_pool_blocks_avail;
	bool should_alloc;
	uint32_t *queues_location;
};

struct zipm_device_data {
	struct device *self;
	sys_dlist_t callbacks;
	int noof_queues;
	struct zipm_shared_queue *queues;
	struct k_sem shared_queue_sem;
	struct k_work work;
};

static int zipm_dev_register_event_callback(const struct device *zdev, struct zipm_callback *cs,
				 zipm_event_callback_t fn, void *user_data)
{
	int key;
	struct zipm_device_data *dev_data = zdev->data;
	
	if(!cs || !fn)
		return -EINVAL;

	cs->callback = fn;
	cs->user_data = user_data;

	key = irq_lock();
	sys_dlist_append(&dev_data->callbacks, &cs->link);
	irq_unlock(key);

	return 0;
}

static int zipm_dev_remove_event_callback(struct zipm_callback *cs)
{
	int key;

	if(!cs)
		return -EINVAL;

	key = irq_lock();
	sys_dlist_remove(&cs->link);
	irq_unlock(key);

	return 0;
}

static int zipm_dev_send(const struct device *zdev, const void *data, size_t size,
	      int shared_queue_number, int wait_time)
{
	int ret;
	bool frag = false;
	size_t remaining = size;
	size_t copied = 0;
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;

	if(!data || !size)
		return -EINVAL;

	if(shared_queue_number >= dev_data->noof_queues)
		return -EINVAL;

	if(size > dev_cfg->node_pool->block_size) {
		frag = true;
	}

	if(!frag) {
		struct zipm_node_descriptor * desc = zipm_node_pool_alloc(dev_cfg->node_pool);
		if(!desc) {
			int ret = k_sem_take(&dev_data->shared_queue_sem, K_MSEC(wait_time));
			if(ret < 0) {
				LOG_ERR("Wait for available descriptor timed out");
				return ret;
			}

			desc = zipm_node_pool_alloc(dev_cfg->node_pool);
			if(!desc) {
				LOG_ERR("No descriptor available to send data");
				return -ENOMEM;
			}
		}

		memcpy(&desc->memory, data, size);
		desc->flags |= ZIPM_NODE_FLAGS_END;
		
		ret = zipm_shared_queue_push(&dev_data->queues[shared_queue_number], desc);
		if(ret < 0) {
			LOG_ERR("failed to send data to the shared queue %d, error: %d", shared_queue_number, ret);
			return ret;
		}

		zipm_shared_queue_set_event(&dev_data->queues[shared_queue_number], ZIPM_DATA_PRODUCED);
		return ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);	
	}
	
	do {
		ret = k_sem_take(&dev_data->shared_queue_sem, K_MSEC(wait_time));
		if(ret < 0) {
			LOG_ERR("Wait for available descriptor timed out");
			return ret;
		}

		struct zipm_node_descriptor * desc = zipm_node_pool_alloc(dev_cfg->node_pool);
		if(!desc)
			continue;

		LOG_DBG("Block allocated! remaining to transfer: %d", remaining);

		if(remaining < dev_cfg->node_pool->block_size) {
			memcpy(&desc->memory, &data+copied, remaining);
			copied += remaining;
			desc->flags |= ZIPM_NODE_FLAGS_NEXT;
		} else {
			memcpy(&desc->memory, &data+copied, dev_cfg->node_pool->block_size);
			copied += dev_cfg->node_pool->block_size;
			desc->flags |= ZIPM_NODE_FLAGS_END;
		}
		
		remaining -= dev_cfg->node_pool->block_size;
		if(remaining < 0) {
			remaining = 0;
		} 

		ret = zipm_shared_queue_push(&dev_data->queues[shared_queue_number], desc);
		if(ret < 0) {
			LOG_ERR("failed to send data to the shared queue %d, error: %d", shared_queue_number, ret);
			return ret;
		}

	} while (remaining);

	zipm_shared_queue_set_event(&dev_data->queues[shared_queue_number], ZIPM_DATA_PRODUCED);
	return ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);
}

static int zipm_dev_receive(const struct device *zdev, void *data, size_t size,
		 int shared_queue_number)
{
	int fragmented;
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;

	if(!data || !size)
		return -EINVAL;

	if(shared_queue_number >= dev_data->noof_queues)
		return -EINVAL;

	if(size < dev_cfg->node_pool->block_size) {
		LOG_ERR("Not enough space provided to extract block of data");
		return -EINVAL;		
	}

	struct zipm_node_descriptor *desc = zipm_shared_queue_get(&dev_data->queues[shared_queue_number]);
	if(!desc) {
		LOG_WRN("No data available, queue seems to be empty");
		return -ENOMEM;
	}

	memcpy(data, &desc->memory, dev_cfg->node_pool->block_size);
	fragmented = (desc->flags & ZIPM_NODE_FLAGS_NEXT);
	if(!fragmented) {
		zipm_shared_queue_set_event(&dev_data->queues[shared_queue_number], ZIPM_DATA_CONSUMED);
		ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);
	}

	zipm_node_pool_dealloc(dev_cfg->node_pool, desc);
	zipm_shared_queue_set_event(&dev_data->queues[shared_queue_number], ZIPM_NODE_AVAIL);
	ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);

	return fragmented;
}

static int zipm_dev_send_doorbell(const struct device *zdev, int shared_queue_number)
{
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;

	if(shared_queue_number >= dev_data->noof_queues) {
		return -EINVAL;
	}

	zipm_shared_queue_set_event(&dev_data->queues[shared_queue_number], ZIPM_DOORBELL);
	return 	ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);
}

static int zipm_dev_flush(const struct device *zdev, int shared_queue_number)
{
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;
	struct zipm_node_descriptor *desc;

	if(shared_queue_number >= dev_data->noof_queues) {
		return -EINVAL;
	}

	do {
		desc = zipm_shared_queue_get(&dev_data->queues[shared_queue_number]);
		LOG_DBG("Deallocating block %p from shared queue %d", desc, shared_queue_number);
		zipm_node_pool_dealloc(dev_cfg->node_pool, desc);
	} while(desc != NULL);

	zipm_shared_queue_set_event(&dev_data->queues[shared_queue_number], ZIPM_NODE_AVAIL);
	return ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);
}

static void zipm_dev_ipm_isr(const struct device *ipcdev, void *user_data,
			       uint32_t id, volatile void *data)
{
	ARG_UNUSED(ipcdev);
	const struct device *zdev = user_data;
	struct zipm_device_data *dev_data = zdev->data;
	dev_data->self = (struct device *)zdev;
	k_work_submit(&dev_data->work);	
}

static void zipm_dev_work_handler(struct k_work *work)
{
	struct zipm_device_data *dev_data = CONTAINER_OF(work, struct zipm_device_data, work);
	const struct device* zdev = dev_data->self;

	for(int i = 0; i < dev_data->noof_queues; i++) {
		enum zipm_event e = zipm_shared_queue_get_event(&dev_data->queues[i]);

		if(e != ZIPM_IDLE) {
			sys_dnode_t *node;
			SYS_DLIST_FOR_EACH_NODE(&dev_data->callbacks, node) {
				struct zipm_callback *cb = CONTAINER_OF(node, struct zipm_callback, link);
				if(cb->callback) {
					cb->callback(zdev, i, cb->user_data, e);
				}
			}
		}

		if(e == ZIPM_DOORBELL) {
			zipm_shared_queue_set_event(&dev_data->queues[i], ZIPM_DOORBELL);
		}

		if(e == ZIPM_NODE_AVAIL) {
			k_sem_give(&dev_data->shared_queue_sem);
		}
	}
}

static int zipm_dev_init(const struct device *zdev)
{
	bool valid;
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;
	int ret;
	
	if(dev_cfg->should_alloc) {
		ret = zipm_node_pool_initialize(&dev_cfg->node_pool, dev_cfg->node_pool_block_size,
			      dev_cfg->node_pool_blocks_avail, (void *)dev_cfg->node_pool_location);
		
		if(ret < 0) {
			LOG_ERR("Failed do initialize node pool at location %p", (void *)dev_cfg->node_pool_location);
			return ret;
		}

		for(int i = 0 ; i < dev_data->noof_queues; i++) {
			ret = zipm_shared_queue_initialize(&dev_data->queues[i], (void *)dev_cfg->queues_location[i]);
			if(ret < 0) {

				LOG_ERR("Failed do initialize shared queue at %p", (void *)dev_cfg->queues_location[i]);
				return ret;
			}
		}
	} else {
		for(int i = 0 ; i < dev_data->noof_queues; i++) {
			dev_data->queues[i] =  *((struct zipm_shared_queue *)dev_cfg->queues_location[i]);
		}

		/* If the device has no allocator role waits until the side
		 * has its role to initialize the shared memory area and 
		 * keeps spinning while it waits for usage
		 */

		do {
			valid = zipm_node_pool_is_valid(dev_cfg->node_pool);
			if(!valid) {
				k_msleep(10);
				continue;
			}

			for(int i = 0 ; i < dev_data->noof_queues; i++) {
				valid = zipm_is_shared_queue_valid(&dev_data->queues[i]);
				if(!valid) {
					k_msleep(10);
					break;
				}
			}
		} while(valid == false);
	}

	LOG_DBG("Found node-pool at location %p", dev_cfg->node_pool);
	LOG_DBG("Block size is %u bytes", dev_cfg->node_pool->block_size);
	LOG_DBG("Blocks available %u", dev_cfg->node_pool->blocks_avail);

	for(int i = 0 ; i < dev_data->noof_queues; i++) {
		LOG_DBG("Found the shared queue %d at %p", i, &dev_data->queues[i]);
	}

	k_sem_init(&dev_data->shared_queue_sem, 0, 1);
	k_work_init(&dev_data->work, zipm_dev_work_handler);
	ipm_register_callback(dev_cfg->ipc_device, zipm_dev_ipm_isr, (void *)zdev);

	return 0;
}
