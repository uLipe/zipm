# Copyright (c) 2024 Felipe Neves
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ZIPM)

zephyr_interface_library_named(ZIPM)
zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(src/zipm.c)
zephyr_library_sources(src/zipm_node_pool.c)
zephyr_library_sources(src/zipm_shared_queue.c)
zephyr_library_sources(src/zipm_backend_zephyr.c)

zephyr_library_link_libraries(ZIPM)
target_link_libraries(ZIPM INTERFACE zephyr_interface)

endif() # CONFIG_ZIPM
