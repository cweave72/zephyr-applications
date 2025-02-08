cmake_minimum_required(VERSION 3.13.1)
message("ZEPHYR_BASE = $ENV{ZEPHYR_BASE}")

list(APPEND EXTRA_CONF_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/nv.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/net.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/modules.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(echo_server)

target_sources(app PRIVATE src/main.c)

target_compile_options(
    app
    PUBLIC
    -fmax-errors=5
)
