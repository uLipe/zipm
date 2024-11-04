# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zipm_tx)

target_sources(app PRIVATE src/main.c)

# Temporary solution for esp32 variants until its AMP gets fixed on zephyr tree:
zephyr_sources_ifdef(CONFIG_BOARD_ESP32S3_DEVKITM_ESP32S3_PROCPU src/zephyr.c)
