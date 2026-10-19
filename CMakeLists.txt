cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mini_shell)

target_sources(app PRIVATE
    src/main.c
    src/board_io.c
    src/command.c
    src/line.c
    src/transport.c
)