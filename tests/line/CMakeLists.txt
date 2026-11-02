# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(line_test)

set(LINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/line)

FILE(GLOB test_sources src/*.c)
target_sources(app PRIVATE ${test_sources} ${LINE_DIR}/line.c)
target_include_directories(app PRIVATE ${LINE_DIR})
