# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(unit_tests)

set(APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

target_include_directories(app PRIVATE ${APP_SRC})
target_sources(app PRIVATE
	src/test_cmd.c
	src/test_line.c
	${APP_SRC}/cmd.c
	${APP_SRC}/line.c
)
