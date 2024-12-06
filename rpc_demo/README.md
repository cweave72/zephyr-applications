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
