cmake_minimum_required(VERSION 3.28.0)

# usa o mesmo Kconfig da aplicacao (MINISHELL_*)
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(minishell_unit)

# board_io real e trocado por um stub, assim o teste nao depende de GPIO
target_sources(app PRIVATE
	src/main.c
	src/board_io_stub.c
	../../src/cmd.c
	../../src/line.c
)

target_include_directories(app PRIVATE ../../src)
