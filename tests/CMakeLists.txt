# Copyright 2024 Felipe Neves
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zipm_test)
enable_language(C ASM)
target_sources(app PRIVATE src/main.c)
