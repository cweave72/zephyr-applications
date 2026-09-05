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
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/tracing.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/tracemodule.conf
)

# Which networking fragment to apply. Declare the type with
# CONFIG_APP_NET_TYPE_<X>=y in boards/<board>.conf (or prj.conf); the helper reads
# that declaration here, because CONFIG_* are not CMake variables until
# find_package(Zephyr) below has run Kconfig -- by which point EXTRA_CONF_FILE is
# already consumed. See the header of app_net_type.cmake.
#
# The "wifi" fallback must match the 'choice APP_NET_TYPE' default in Kconfig.
include($ENV{WORKSPACE_BASE}/common/scripts/cmake/app_net_type.cmake)
app_net_type_resolve(APP_NET_TYPE wifi)

if (APP_NET_TYPE STREQUAL "wifi")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/nv.conf)
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/wifi.conf)
elseif (APP_NET_TYPE STREQUAL "serial")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/serial_net.conf)
elseif (APP_NET_TYPE STREQUAL "eth")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/wired_eth_net.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Kconfig has now run. Fail loudly if it disagrees with the fragment we applied.
app_net_type_verify(${APP_NET_TYPE})

project(rpc_demo)


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
    )

target_sources_ifdef(
    CONFIG_APP_NET_TYPE_ETH
    ${target}
    PRIVATE 
    src/wired_net.c
    )

target_sources_ifdef(
    CONFIG_TRACERAM
    ${target}
    PRIVATE 
    $ENV{WORKSPACE_BASE}/common/modules/TraceRam/src/TraceRam_backend.c
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
