# Application: Blinky

Simple app demonstrating a few key concepts:
* Devicetree overlays
* Threads
* Event flags

It is assumed that this app resides within a workspace repo (i.e.
zephyr-workspace)

## Building the app

To build the app, run the following:
```bash
./buildall --cmd=build --board=<BOARD>
```

Available boards:
* `esp32_devkitc_wroom`
