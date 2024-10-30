/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZIPM_CORE_PORTABLE_H
#define __ZIPM_CORE_PORTABLE_H

#define ZIPM_SHM_LOCK_FREE	0x4d50495a
#define ZIPM_SHM_LOCK_TOOK 	0x5a49504d


#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

static inline void zipm_zephyr_spin_lock_shm(volatile const atomic_t *control)
{
	int key = irq_lock();
	while (!atomic_cas((atomic_t *)control, ZIPM_SHM_LOCK_FREE,
			   ZIPM_SHM_LOCK_TOOK)) {
		;
	}
	irq_unlock(key);
}

static inline void zipm_zephyr_spin_unlock_shm(volatile const atomic_t *control)
{
	int key = irq_lock();
	atomic_set((atomic_t *)control, ZIPM_SHM_LOCK_FREE);
	irq_unlock(key);
}

#define ZIPM_SPIN_LOCK_SHM(x) zipm_zephyr_spin_lock_shm((volatile const atomic_t *)x)
#define ZIPM_SPIN_UNLOCK_SHM(x) zipm_zephyr_spin_unlock_shm((volatile const atomic_t *)x)
#define ZIPM_IRQ_LOCK() int key = irq_lock()
#define ZIPM_IRQ_UNLOCK() irq_unlock(key)
#define ZIPM_ATOMIC_SET(x, val) atomic_set((atomic_t *)x, val)

#else

#warning "You are running zipm core outside of zephyr, please implement the porting layer!" 

#define ZIPM_IRQ_LOCK()
#define ZIPM_IRQ_UNLOCK()
#define ZIPM_SPIN_LOCK_SHM(x)
#define ZIPM_SPIN_UNLOCK_SHM(x)
#define ZIPM_ATOMIC_SET(x, val)

#define __packed __attribute__(("packed"))

#include "zephyr_list_oot.h"

#endif
#endif

