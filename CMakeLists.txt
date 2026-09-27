# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(command)

file(GLOB SOURCES_APP src/*.c)

add_library(command_lib STATIC ${SOURCES_APP})
target_include_directories(command_lib PUBLIC include/)
target_link_libraries(command_lib PUBLIC zephyr_interface)

target_link_libraries(app PRIVATE command_lib)

target_sources(app PRIVATE ${SOURCES_APP})
