# RPC Demo app

This app demonstrates using the ProtoRpc library for executing RPCs.

## Configuration

The app supports the following CONFIGs for network type:
* CONFIG_APP_NET_TYPE_WIFI : Uses wifi.
* CONFIG_APP_NET_TYPE_SERIAL : Uses eth-serial

## Wifi Networking

Build:
```bash
make build BOARD=<board> CMAKE_OPTS="-DCONFIG_APP_NET_TYPE_WIFI=y"
```

See `boards/` for supported ESP32 boards.

Example: Using the 01space esp32c4 .042 OLED board:
```bash
make build BOARD=esp32c3_042_oled CMAKE_OPTS="-DCONFIG_APP_NET_TYPE_WIFI=y"
```

Example: Using the esp32 matrix:
```bash
make build BOARD=esp32s3_matrix/esp32s3/procpu"
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
