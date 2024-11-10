# Application: Wifi Station

App demonstrating connecting to a Wifi access point.

It is assumed that this app resides within a workspace repo (i.e.
zephyr-workspace)

## Building the app

To build the app, run the following:
```bash
./buildall --cmd=build --board=<BOARD>
```
Example: Build for esp32s3_qtpy
```
./buildall.sh --cmd=build --board=esp32s3_qtpy/esp32s3/procpu
```

## Flashing the app (and run the monitor)

To build the app, run the following:
```bash
./buildall --cmd=flashmon
```


## Setting the wifi ssid and password

Use the settings shell to set the ssid and password:

```
uart:~$ setting write string ssid <enter ssid>
uart:~$ setting write string pass <enter password>
```

Then reboot for connection.

## Things to try

Look at network interface config (shell):  `net iface`

iperf to host:

On linux host: (tested with iperf version 2.1.5)
```
iperf -s
```

On zephyr shell:
```
zperf tcp upload <host ip> 5001 10 1K
```

Available boards:
* `esp32_devkitc_wroom`
* `esp32s3_matrix`
* `esp32s3_qtpy`
