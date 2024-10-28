/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zipm_message_device

#include <errno.h>
#include <zephyr/cache.h>
#include <zipm/zipm.h>
#include "zipm_node_pool.h"
#include "zipm_shared_queue.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zipm, CONFIG_LOG_DEFAULT_LEVEL);

struct zipm_device_config {
	const struct device *ipc_device;
	uint32_t node_pool_location;
	uint32_t node_pool_block_size;
	uint32_t node_pool_blocks_avail;
	bool should_init;
	uint32_t *queues_location;
};

struct zipm_device_data {
	struct zipm_node_pool_header* node_pool;
	struct device *self;
	sys_dlist_t callbacks;
	int noof_queues;
	struct k_sem shared_queue_sem;
	struct k_work work_link;
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
	const struct zipm_device_config *dev_cfg = zdev->config;
	struct zipm_shared_queue *sq;
	struct zipm_node_descriptor desc;
	uint8_t *mem;

	if(shared_queue_number >= dev_data->noof_queues)
		return -EINVAL;

	if(size > dev_data->node_pool->block_size && data) {
		frag = true;
	}

	sq = zipm_shared_queue_access((void *)dev_cfg->queues_location[shared_queue_number]);
	if(!sq) {
		LOG_ERR("Invalid shared queue!");
		return -EINVAL;
	}

	if(!frag) {
		mem = zipm_node_pool_alloc(dev_data->node_pool);
		if(!mem) {
			int ret = k_sem_take(&dev_data->shared_queue_sem, K_MSEC(wait_time));
			if(ret < 0) {
				LOG_ERR("Wait for available descriptor timed out");
				return ret;
			}

			mem = zipm_node_pool_alloc(dev_data->node_pool);
			if(!mem) {
				LOG_ERR("No descriptor available to send data");
				return -ENOMEM;
			}
		}

		desc.addr = (uint32_t)mem;
		desc.size = dev_cfg->node_pool_block_size;
		if(data && size) {
			memcpy(mem, data, size);
			desc.flags = ZIPM_NODE_FLAGS_END;
		} else {
			desc.flags = (ZIPM_NODE_FLAGS_END | ZIPM_NODE_FLAGS_EMPTY);
		}

		ret = zipm_shared_queue_push(sq, &desc);
		if(ret < 0) {
			LOG_ERR("failed to send data to the shared queue %d, error: %d", shared_queue_number, ret);
			return ret;
		}

		return ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);	
	}
	
	do {
		uintptr_t from = (uintptr_t)data;
		mem = zipm_node_pool_alloc(dev_data->node_pool);
		if(!mem) {
			ret = k_sem_take(&dev_data->shared_queue_sem, K_MSEC(wait_time));
			if(ret < 0) {
				LOG_ERR("Wait for available descriptor timed out");
				return ret;
			}

			continue;
		}

		LOG_DBG("Block allocated! remaining to transfer: %d\n", remaining);

		desc.addr = (uint32_t)mem;
		desc.size = dev_cfg->node_pool_block_size;
		if(remaining > desc.size) {
			memcpy((void *)mem, (const void *)(from+copied), desc.size);
			copied += desc.size;
			desc.flags = ZIPM_NODE_FLAGS_NEXT;
			remaining -= desc.size;
		} else {
			memcpy((void *)mem, (const void *)(from+copied), remaining);
			copied += remaining;
			desc.flags = ZIPM_NODE_FLAGS_END;
			remaining = 0;
		}
		
		ret = zipm_shared_queue_push(sq, &desc);
		if(ret < 0) {
			LOG_ERR("failed to send data to the shared queue %d, error: %d", shared_queue_number, ret);
			return ret;
		}

	} while (remaining);
	return ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);
}

static int zipm_dev_receive(const struct device *zdev, void *data, size_t *size,
		 int shared_queue_number)
{
	int fragmented;
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;
	struct zipm_node_descriptor desc;
	struct zipm_shared_queue *sq;

	if(shared_queue_number >= dev_data->noof_queues)
		return -EINVAL;

	sq = zipm_shared_queue_access((void *)dev_cfg->queues_location[shared_queue_number]);
	if(!sq) {
		LOG_ERR("Invalid shared queue!");
		return -EINVAL;
	}

	int ret = zipm_shared_queue_get(sq, &desc);
	if(ret < 0) {
		LOG_DBG("No data available, queue seems to be empty");
		return -ENOMEM;
	}

	if(!(desc.flags & ZIPM_NODE_FLAGS_EMPTY) && data) {
		memcpy(data, (const void *)desc.addr, desc.size);
	}

	fragmented = (desc.flags & ZIPM_NODE_FLAGS_NEXT);

	zipm_node_pool_dealloc(dev_data->node_pool, (uint8_t *)desc.addr);
	k_sem_give(&dev_data->shared_queue_sem);

	if(size != NULL) {
		*size = desc.size;
	}

	return fragmented;
}

static int zipm_dev_flush(const struct device *zdev, int shared_queue_number)
{
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;
	struct zipm_node_descriptor desc;
	struct zipm_shared_queue *sq;
	int ret;

	if(shared_queue_number >= dev_data->noof_queues) {
		return -EINVAL;
	}

	sq = zipm_shared_queue_access((void *)dev_cfg->queues_location[shared_queue_number]);
	if(!sq) {
		LOG_ERR("Invalid shared queue!");
		return -EINVAL;
	}

	do {
		ret = zipm_shared_queue_get(sq, &desc);
		if(!ret) {
			LOG_DBG("Deallocating block %p from shared queue %d \n", (void *)desc.addr, shared_queue_number);
			zipm_node_pool_dealloc(dev_data->node_pool, (uint8_t *)desc.addr);
		}
	} while(!ret);

	k_sem_give(&dev_data->shared_queue_sem);
	return ipm_send(dev_cfg->ipc_device, 0, 0, NULL, 0);
}

static void zipm_dev_ipm_isr(const struct device *ipcdev, void *user_data,
			       uint32_t id, volatile void *data)
{
	ARG_UNUSED(ipcdev);
	const struct device *zdev = user_data;
	struct zipm_device_data *dev_data = zdev->data;
	dev_data->self = (struct device *)zdev;
	k_work_submit(&dev_data->work_link);	
}

static void zipm_dev_work_handler(struct k_work *work)
{
	struct zipm_device_data *dev_data = CONTAINER_OF(work, struct zipm_device_data, work_link);
	const struct device* zdev = dev_data->self;
	const struct zipm_device_config* dev_cfg = dev_data->self->config;
	struct zipm_shared_queue *sq;
	sys_dnode_t *node;

	for(int i = 0; i < dev_data->noof_queues; i++) {

		sq = zipm_shared_queue_access((void *)dev_cfg->queues_location[i]);
		if(!sq) {
			LOG_ERR("Invalid shared queue!");
			continue;
		}

		if(!zipm_shared_queue_has_data(sq))
			continue;

		SYS_DLIST_FOR_EACH_NODE(&dev_data->callbacks, node) {
			struct zipm_callback *cb = CONTAINER_OF(node, struct zipm_callback, link);
			if(cb->callback) {
				cb->callback(zdev, i, cb->user_data);
			}
		}
	}
}

static int zipm_dev_init(const struct device *zdev)
{
	struct zipm_device_data *dev_data = zdev->data;
	const struct zipm_device_config  *dev_cfg = zdev->config;
	int ret;

	if(dev_cfg->should_init) {
		ret = zipm_node_pool_initialize(dev_cfg->node_pool_block_size,
			      dev_cfg->node_pool_blocks_avail, (void *)dev_cfg->node_pool_location);
		
		if(ret < 0) {
			LOG_ERR("Failed do initialize node pool at location %p", (void *)dev_cfg->node_pool_location);
			return ret;
		}

		for(int i = 0 ; i < dev_data->noof_queues; i++) {
			ret = zipm_shared_queue_initialize((void *)dev_cfg->queues_location[i],
							   dev_cfg->node_pool_blocks_avail);
			if(ret < 0) {

				LOG_ERR("Failed do initialize shared queue at %p", (void *)dev_cfg->queues_location[i]);
				return ret;
			}
		}
	}

	dev_data->node_pool = zipm_node_pool_access((void *)dev_cfg->node_pool_location);
	if(dev_data->node_pool == NULL) {
		LOG_ERR("Invalid node pool! aborting!");
		return -ENOENT;
	}

	LOG_DBG("Found node-pool at location %p", dev_data->node_pool);
	LOG_DBG("Block size is %u bytes", dev_data->node_pool->block_size);
	LOG_DBG("Blocks available %u", dev_data->node_pool->blocks_avail);

	k_sem_init(&dev_data->shared_queue_sem, 0, 1);
	k_work_init(&dev_data->work_link, zipm_dev_work_handler);
	sys_dlist_init(&dev_data->callbacks);
	ipm_register_callback(dev_cfg->ipc_device, zipm_dev_ipm_isr, (void *)zdev);
	ipm_set_enabled(dev_cfg->ipc_device, 1);

	return 0;
}

static const struct zipm_device_api api = {
	.register_callback = zipm_dev_register_event_callback,
	.remove_callback = zipm_dev_remove_event_callback,
	.send = zipm_dev_send,
	.receive = zipm_dev_receive,
	.flush = zipm_dev_flush,
};

#define SHARED_QUEUES_REG_BY_IDX(node_id, prop, idx) \
	DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define ZIPM_DEV_INIT(n)	\
static uint32_t zipm_queues_addr_##n[DT_INST_PROP_LEN(n, shared_queues)] = {			\
	DT_FOREACH_PROP_ELEM_SEP(DT_INST(n, DT_DRV_COMPAT), shared_queues,SHARED_QUEUES_REG_BY_IDX, (,))	\
};																						\
																						\
static const  struct zipm_device_config zipm_cfg_##n = {								\
	.ipc_device = DEVICE_DT_GET(DT_INST_PROP(n, ipc)),									\
	.node_pool_location = DT_REG_ADDR(DT_INST_PROP(n, node_pool)),						\
	.queues_location = &zipm_queues_addr_##n[0],										\
	.node_pool_location = DT_REG_ADDR(DT_INST_PROP(n, node_pool)),						\
	.node_pool_block_size = DT_PROP(DT_INST_PROP(n, node_pool), block_size),			\
	.node_pool_blocks_avail =  DT_PROP(DT_INST_PROP(n, node_pool),nodes_quantity), 		\
	.should_init = DT_INST_PROP(n, should_init),										\
};																						\
																						\
static struct zipm_device_data zipm_dev_data_##n = {									\
	.noof_queues = DT_INST_PROP_LEN(n, shared_queues),									\
};																						\
																						\
DEVICE_DT_INST_DEFINE(n, &zipm_dev_init, NULL, &zipm_dev_data_##n, &zipm_cfg_##n,      	\
				POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ZIPM_DEV_INIT)
