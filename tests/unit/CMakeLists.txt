# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(minish_unit)

set(MINISH_SRC ${CMAKE_CURRENT_LIST_DIR}/../../src)

target_sources(app PRIVATE
  src/main.c
  ${MINISH_SRC}/dispatch.c
  ${MINISH_SRC}/line_editor.c
)
target_include_directories(app PRIVATE ${MINISH_SRC})
