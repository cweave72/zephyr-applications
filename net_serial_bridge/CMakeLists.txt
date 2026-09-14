cmake_minimum_required(VERSION 3.13.1)
message("WORKSPACE_BASE = $ENV{WORKSPACE_BASE}")
message("ZEPHYR_BASE = $ENV{ZEPHYR_BASE}")
message("ZEPHYR_SDK_INSTALL_DIR = $ENV{ZEPHYR_SDK_INSTALL_DIR}")
message("ZEPHYR_TOOLCHAIN_VARIANT = $ENV{ZEPHYR_TOOLCHAIN_VARIANT}")

set(CONFIG_OUTPUT_DISASSEMBLY y CACHE INTERNAL "")
set(CONFIG_OUTPUT_DISASSEMBLY_WITH_SOURCE y CACHE INTERNAL "")

list(APPEND EXTRA_CONF_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/kernel.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/modules.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/led.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/net.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/conf/rpc.conf
)

# Which networking fragment to apply. Declare the type with
# CONFIG_APP_NET_TYPE_<X>=y in boards/<board>.conf (or prj.conf); the helper reads
# that declaration here, because CONFIG_* are not CMake variables until
# find_package(Zephyr) below has run Kconfig -- by which point EXTRA_CONF_FILE is
# already consumed. See the header of app_net_type.cmake.
#
# The "eth" fallback must match the 'choice APP_NET_TYPE' default in Kconfig.
include($ENV{WORKSPACE_BASE}/common/scripts/cmake/app_net_type.cmake)
app_net_type_resolve(APP_NET_TYPE eth)

if (APP_NET_TYPE STREQUAL "wifi")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/wifi.conf)
elseif (APP_NET_TYPE STREQUAL "serial")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/serial_net.conf)
elseif (APP_NET_TYPE STREQUAL "eth")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/wired_eth_net.conf)
elseif (APP_NET_TYPE STREQUAL "usb")
    list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/usb_net.conf)
endif()

# Which serial port the bridge remotes. A devicetree overlay cannot be selected
# by Kconfig, so this is a CMake cache variable that picks both the conf
# fragment and the overlay. Kconfig 'choice APP_SERIAL_SRC' mirrors it and is
# verified after find_package(Zephyr) below. Switching needs PRISTINE=y.
#
#   make BOARD=w55rp20_evb_pico PRISTINE=y build                                  # uart0
#   make BOARD=w55rp20_evb_pico PRISTINE=y CMAKE_OPTS="-DAPP_SERIAL_SRC=cdc_acm" build
set(APP_SERIAL_SRC "uart0" CACHE STRING "Serial port bridged to TCP: uart0 | cdc_acm")
if (APP_SERIAL_SRC STREQUAL "uart0")
    list(APPEND EXTRA_CONF_FILE        ${CMAKE_CURRENT_SOURCE_DIR}/conf/serial_uart0.conf)
    list(APPEND EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/serial_uart0.overlay)
elseif (APP_SERIAL_SRC STREQUAL "cdc_acm")
    list(APPEND EXTRA_CONF_FILE        ${CMAKE_CURRENT_SOURCE_DIR}/conf/serial_cdc_acm.conf)
    list(APPEND EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/serial_cdc_acm.overlay)
else()
    message(FATAL_ERROR "[app] APP_SERIAL_SRC='${APP_SERIAL_SRC}'; expected uart0 or cdc_acm.")
endif()
message(STATUS "[app] Bridged serial source: ${APP_SERIAL_SRC}")

# Applied LAST so the addressing decision cannot be overridden by a transport
# fragment. CONFIG_NET_DHCPV4 and the APP_IPV4_* symbols live only in this file.
list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conf/ip.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Kconfig has now run. Fail loudly if it disagrees with the fragment we applied.
app_net_type_verify(${APP_NET_TYPE})

if ((APP_SERIAL_SRC STREQUAL "uart0"   AND NOT CONFIG_APP_SERIAL_SRC_UART0) OR
    (APP_SERIAL_SRC STREQUAL "cdc_acm" AND NOT CONFIG_APP_SERIAL_SRC_CDC_ACM))
    message(FATAL_ERROR "[app] APP_SERIAL_SRC='${APP_SERIAL_SRC}' disagrees with Kconfig.")
endif()
if (NOT CONFIG_SERIALPIPE)
    message(FATAL_ERROR "[app] CONFIG_SERIALPIPE is not set. Check its Kconfig dependencies.")
endif()

project(net_serial_bridge)

set(target app)

# Include the cmake script which builds protobuf bindings.
include($ENV{WORKSPACE_BASE}/common/scripts/cmake/build_proto.cmake)

# The following creates the target ${target}_proto.
nanopb_build_sources(${target}
    $ENV{WORKSPACE_BASE}/proto
    $ENV{WORKSPACE_BASE}/applications/net_serial_bridge/proto
)

target_sources(
    ${target}
    PRIVATE
    src/main.c
    src/led.c
    src/rpc.c
    )

# The bridge needs both SerialPipe and TcpServer; app.h stubs app_bridge_init()
# otherwise.
target_sources_ifdef(
    CONFIG_SERIALPIPE
    ${target}
    PRIVATE
    src/bridge_serial.c
    src/bridge_tcp.c
    )

# Exactly one net_*.c provides app_net_init(). net_l4.c carries the bring-up
# sequence shared by the three transports that use Zephyr's connection manager.
target_sources_ifdef(
    CONFIG_APP_NET_TYPE_WIFI
    ${target}
    PRIVATE
    src/net_wifi.c
    )

target_sources_ifdef(
    CONFIG_APP_NET_TYPE_SERIAL
    ${target}
    PRIVATE
    src/net_wired.c
    src/net_l4.c
    )

target_sources_ifdef(
    CONFIG_APP_NET_TYPE_ETH
    ${target}
    PRIVATE
    src/net_wired.c
    src/net_l4.c
    )

target_sources_ifdef(
    CONFIG_APP_NET_TYPE_USB
    ${target}
    PRIVATE
    src/net_usb.c
    src/net_l4.c
    )

target_sources_ifdef(
    CONFIG_APP_USE_STATIC_IP
    ${target}
    PRIVATE
    src/net_ip.c
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

