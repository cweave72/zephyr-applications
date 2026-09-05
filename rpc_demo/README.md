# RPC Demo app

This app demonstrates using the ProtoRpc library for executing RPCs.

## Configuration

The app supports three network types, one per `conf/` fragment:

| Symbol | Transport | Fragment applied |
|---|---|---|
| `CONFIG_APP_NET_TYPE_WIFI` | wifi | `conf/wifi.conf` + `conf/nv.conf` |
| `CONFIG_APP_NET_TYPE_SERIAL` | eth-serial | `conf/serial_net.conf` |
| `CONFIG_APP_NET_TYPE_ETH` | wired ethernet | `conf/wired_eth_net.conf` |

**Declare the type in the board's own fragment**, `boards/<board>.conf` — one line,
and nothing is needed on the make command line:

```
CONFIG_APP_NET_TYPE_ETH=y
```

`boards/w55rp20_evb_pico.conf` does exactly this. Put the line in `prj.conf` instead
to set an app-wide default; with no declaration anywhere the build falls back to wifi.
The build prints which type it chose and where the declaration came from:

```
-- [app] Using wired ethernet networking (from boards/w55rp20_evb_pico.conf).
```

For a one-off override, the command-line form still works and takes precedence over
the board fragment:

```bash
make build BOARD=<board> CMAKE_OPTS="-DCONFIG_APP_NET_TYPE_SERIAL=y"
```

Note the declaration must be a real Kconfig line, because it does double duty: it sets
the Kconfig symbol *and* tells CMake which `conf/` fragment to apply. CMake cannot read
Kconfig symbols (they do not exist until `find_package(Zephyr)` has run, by which point
the fragment list is already fixed), so `common/scripts/cmake/app_net_type.cmake` reads
the declaration directly and then asserts after Kconfig that the two agree. A mismatch
is a hard build error rather than a silently wrong config.

## Wifi Networking

Build:
```bash
make build BOARD=<board>
```

See `boards/` for supported ESP32 boards.

Example: Using the 01space esp32c4 .042 OLED board:
```bash
make build BOARD=esp32c3_042_oled
```

Example: Using the esp32 matrix:
```bash
make build BOARD=esp32s3_matrix/esp32s3/procpu
```

Flash the board (and optionally, run the monitor):
```bash
make flash [mon]
```

## Serial Networking

Build (supports qemu_x86_64):
```bash
make build BOARD=qemu_x86_64 CMAKE_OPTS="-DCONFIG_APP_NET_TYPE_SERIAL=y"
```

To make that the permanent choice for the board, put
`CONFIG_APP_NET_TYPE_SERIAL=y` in `boards/qemu_x86_64.conf` instead and just run
`make build BOARD=qemu_x86_64`.

### Setting up Serial Networking with QEMU

TODO


## Running the python test app

```
cd <workspace root>/python
uv sync --reinstall
. init_venv.sh
```

To display running threads (example board at IP 192.168.1.168:
```
(.venv) rtosutils-cli --ip 192.168.1.168 -c <path/to/applications>/rpc_demo/callsets.yaml get-tasks
```

## Debugging

### ESP32C3 (gdb over usb)

Download espresif custom openocd:

https://github.com/espressif/openocd-esp32/releases (get latest)

Install locally: `~/.local/opt`
```
cd ~/.local/opt
cp ~/Downloads/openocd-esp32-linux-amd64-0.12.0-esp32-20241016.tar.gz .
tar xvf openocd-esp32-linux-amd64-0.12.0-esp32-20241016.tar.gz
```

**NOTE:** Might need to open up usb for non-root users if you get
libusb error: LIBUSB_ERROR_ACCESS. Run:
`sudo chmod -R 777 /dev/bus/usb/`

Building with openocd support:
```
make BOARD=esp32c3_042_oled build ARGS="-- -DOPENOCD=~/.local/opt/openocd-esp32/bin/openocd -DOPENOCD_DEFAULT_PATH=~/.local/opt/openocd-esp32/share/openocd/scripts"
```

Then flash and debug:
```
make flash
make west ARGS=debug
```

### W55RP20-EVB-Pico (SWD via Raspberry Pi Debug Probe)

Unlike the ESP32 recipe above, nothing has to be baked into the build: the make
flow hands west the right OpenOCD at flash/debug time, from
`common/boards/wiznet/w55rp20_evb_pico/board.mk`.

```
make BOARD=w55rp20_evb_pico PRISTINE=y SNIPPET=debug build
make flash          # over SWD; no BOOTSEL and no UF2 mount
make debug          # gdb, stopped at main, with Zephyr thread awareness
```

One-time host setup (build Raspberry Pi's OpenOCD fork, install the udev rule),
the wiring, and the board-specific gotchas are documented in
`common/boards/wiznet/w55rp20_evb_pico/README.md`.

The network type comes from `boards/w55rp20_evb_pico.conf`, so nothing extra is
needed on the command line — see [Configuration](#configuration).
