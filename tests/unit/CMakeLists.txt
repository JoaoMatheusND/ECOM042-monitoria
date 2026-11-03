# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_ctrl_unit)

# Testa o módulo do app (src/led_ctrl.c) com LEDs em GPIOs emulados.
set(APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

target_include_directories(app PRIVATE ${APP_SRC})
target_sources(app PRIVATE
  src/test_led_ctrl.c
  ${APP_SRC}/led_ctrl.c
)
