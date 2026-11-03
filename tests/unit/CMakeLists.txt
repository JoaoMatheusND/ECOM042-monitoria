cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_shell)

target_sources(app PRIVATE
    src/test_shell.c
    ../../src/shell.c
)

target_include_directories(app PRIVATE ../../src)
