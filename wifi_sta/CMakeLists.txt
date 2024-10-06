cmake_minimum_required(VERSION 3.13.1)
message("ZEPHYR_BASE = $ENV{ZEPHYR_BASE}")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_sta)

target_sources(app PRIVATE src/main.c)

target_compile_options(
    app
    PUBLIC
    -fmax-errors=5
    )
