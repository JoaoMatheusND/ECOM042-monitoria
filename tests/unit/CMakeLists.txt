# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# Usa o mesmo Kconfig do app para enxergar CONFIG_MINISHELL_* nos testes.
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(minishell_unit_tests)

target_sources(app PRIVATE
	src/test_main.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../src/line_editor.c
	${CMAKE_CURRENT_SOURCE_DIR}/../../src/minishell.c
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
