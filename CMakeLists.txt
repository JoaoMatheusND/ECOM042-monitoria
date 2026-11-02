# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mini_shell)

target_sources(app PRIVATE
	src/main.c
	src/board.c
	src/shell.c
	src/shell_line.c
	src/uart_shell.c
)
