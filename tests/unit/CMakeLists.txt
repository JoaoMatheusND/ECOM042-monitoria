# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# Mesmo Kconfig do app (periodos dos padroes e nivel de log).
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_unit_tests)

target_sources(app PRIVATE
	src/test_main.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../src/led.c
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
