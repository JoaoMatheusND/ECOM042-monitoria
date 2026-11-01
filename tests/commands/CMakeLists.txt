# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(commands_test)

set(COMMANDS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/commands)

FILE(GLOB test_sources src/*.c)
target_sources(app PRIVATE ${test_sources} ${COMMANDS_DIR}/cmd.c)
target_include_directories(app PRIVATE ${COMMANDS_DIR})
