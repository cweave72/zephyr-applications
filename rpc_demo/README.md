# RPC Demo app

This app demonstrates using the ProtoRpc library for executing RPCs.

## Build Steps

Using the 01space esp32c4 .042 OLED board:
```bash
make BOARD=esp32c3_042_oled build
```

Flash the board (and optionally, run the monitor):
```bash
make flash [mon]
```

Build the python ProtoRpc bindings:
```bash
make proto
```

## Running the python test app

```bash
cd python
. init_venv.sh
test_run --ip <board ip>
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
