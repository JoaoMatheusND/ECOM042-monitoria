# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_unit_test)

set(LED_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/led)

target_sources(app PRIVATE
	${LED_SRC_DIR}/led.c
	${LED_SRC_DIR}/led_cmd.c
)

target_include_directories(app PRIVATE ${LED_SRC_DIR})

if(CONFIG_LED_TEST_NOT_READY)
	target_sources(app PRIVATE src/test_led_not_ready.c)
else()
	target_sources(app PRIVATE src/test_led.c)
endif()
