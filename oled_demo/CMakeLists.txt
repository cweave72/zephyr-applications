cmake_minimum_required(VERSION 3.13.1)
message("WORKSPACE_BASE = $ENV{WORKSPACE_BASE}")
message("ZEPHYR_BASE = $ENV{ZEPHYR_BASE}")
message("ZEPHYR_SDK_INSTALL_DIR = $ENV{ZEPHYR_SDK_INSTALL_DIR}")
message("ZEPHYR_TOOLCHAIN_VARIANT = $ENV{ZEPHYR_TOOLCHAIN_VARIANT}")

set(CONFIG_OUTPUT_DISASSEMBLY y CACHE INTERNAL "")
set(CONFIG_OUTPUT_DISASSEMBLY_WITH_SOURCE y CACHE INTERNAL "")

list(APPEND EXTRA_CONF_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/kernel.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/net.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/modules.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/rpc.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/nv.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/wifi.conf
)

#cmake_minimum_required(VERSION 3.13.1)
#message("ZEPHYR_BASE = $ENV{ZEPHYR_BASE}")
#find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(oled_demo)

set(target app)

# Include the cmake script which builds protobuf bindings.
include($ENV{WORKSPACE_BASE}/common/scripts/cmake/build_proto.cmake)

# The following creates the target ${target}_proto.
nanopb_build_sources(${target} $ENV{WORKSPACE_BASE}/proto)

target_sources(
    ${target}
    PRIVATE
    src/main.c
    src/rpc.c
    src/sensor.c
    src/font5x7.c
    src/font8x8.c
    )

target_compile_options(
    ${target}
    PUBLIC
    -fmax-errors=5
    )

target_link_libraries(
    ${target}
    INTERFACE
    ${target}_proto
    )
