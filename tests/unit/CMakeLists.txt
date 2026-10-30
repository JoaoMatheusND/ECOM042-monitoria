cmake_minimum_required(VERSION 3.28.0)

# usa o mesmo Kconfig da aplicacao (BLINK_*_MS)
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_unit)

target_sources(app PRIVATE
	src/main.c
	../../src/led.c
)

target_include_directories(app PRIVATE ../../src)
