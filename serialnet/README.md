# serialnet - Network over serial port

This application demonstrates network connectivity over UART, which is handled
in the custom driver `eth_serial`.

## Prerequisites

* Some USB to serial converter (i.e. DFRobot FTDI USB to Serial adapter, Digikey
  DFR0065)

* `taptool` see [tuntap-serial](https://github.com/cweave72/tuntap-serial) 

## Configuration

The `serialnet` app uses the `eth_serial` driver, which can be configured to use
one of the following framing methods:

* SLIP (serial-line interface protocol) [slip](https://github.com/cweave72/zephyr-common/tree/main/modules/slip)
* COBS (consistent overhead byte stuffing) [cobs](https;//github.com/cweave72/zephyr-common/tree/main/modules/Cobs)

To us SLIP (in `proj.conf`):
```
CONFIG_ETH_SERIAL_SLIP=y
```

To us COBS:
```
CONFIG_ETH_SERIAL_COBS=y
```

## Build and Flash

Build
```
make build BOARD=esp32_devkitc_wroom/esp32/procpu
```

We have to be a bit more explicit since we have 2 serial ports in use. The
esptool flasher seems to pick the highest number ttyUSBx device.

* ESP32 Devkit: /dev/ttyUSB0  (plugged in first)
* FTDI USB to serial: /dev/ttyUSB1

Flash and monitor:
```
make flash ARGS="--esp-device /dev/ttyUSB0"
make mon ARGS="-p /dev/ttyUSB0"
```

## Running in QEMU

This will allow running the application under QEMU and provide networking with
the host linux PC.

* We must first set up an emulated serial device and listening socket using a
`socat` command. This is accomplished via the script `loop-socat.sh` which is
found either at `deps/tools/net-tools` or wherever you cloned `tuntap-serial`
mentioned above.
```
sudo ./loop-socat.sh
```

This will create a psuedo terminal and link it to the file `/tmp/slip.dev` which
is expected by QEMU. For reference, the socat command is shown below:
```
socat PTY,link=/tmp/slip.dev UNIX-LISTEN:/tmp/slip.sock
```

This command creates a virtual serial port (`/tmp/slip.dev`) and establishes a
Unix socket connection at `/tmp/slip.sock`.

* Build and run the app in `qemu_x86`:
```
make build BOARD=qemu_x86
sudo make west ARGS="build -t run"
```

If you see the following error message, you forgot to run the `loop-socat.sh`
script before running.
```
qemu-system-i386: -serial unix:/tmp/slip.sock: Failed to connect to '/tmp/slip.sock': No such file or directory
qemu-system-i386: -serial unix:/tmp/slip.sock: could not connect serial device to character backend 'unix:/tmp/slip.sock'
```

* Finally, start the `taptool` utility as follows (the `-d` is optional):
```
sudo .venv/bin/taptool -d tap --tty /tmp/slip.dev --ip 192.0.2.2 --slip
```

>**Note**: Notice that running the `loop-socat.sh` script and running the app
>with qemu must be as `sudo`.

* The target app and network are now ready for testing.

## Testing

The build defaults to using an IP of `192.0.2.1`.

Run `taptool` (from wherever tuntap-serial.git was cloned, use option for
framing method chosen):
```
. .venv/bin/activate
(in venv) sudo .venv/bin/taptool -d tap --tty /dev/ttyUSB1 --ip 192.0.2.2 [--slip | --cobs]
```

Interface `tap0` should be active.

On linux host:
```
ping 192.0.2.1
```

### iperf (Host to Embedded)

From Zephyr shell:
```
uart:~$ zperf tcp download 5001
TCP server started on port 5001
[00:00:08.137,000] <inf> net_zperf: Binding to 0.0.0.0
[00:00:08.138,000] <inf> net_zperf: Binding to ::
[00:00:08.138,000] <inf> net_zperf: Listening on port 5001
```

From Linux host:
```
$ iperf -c 192.0.2.1 -i 1
------------------------------------------------------------
Client connecting to 192.0.2.1, TCP port 5001
TCP window size: 85.0 KByte (default)
------------------------------------------------------------
[  1] local 192.0.2.2 port 46756 connected with 192.0.2.1 port 5001
[ ID] Interval       Transfer     Bandwidth
[  1] 0.0000-1.0000 sec  77.1 KBytes   631 Kbits/sec
[  1] 1.0000-2.0000 sec  8.55 KBytes  70.1 Kbits/sec
[  1] 2.0000-3.0000 sec  51.3 KBytes   420 Kbits/sec
[  1] 3.0000-4.0000 sec  77.0 KBytes   631 Kbits/sec
[  1] 4.0000-5.0000 sec  17.1 KBytes   140 Kbits/sec
[  1] 5.0000-6.0000 sec  8.55 KBytes  70.1 Kbits/sec
[  1] 6.0000-7.0000 sec  17.1 KBytes   140 Kbits/sec
[  1] 7.0000-8.0000 sec  8.55 KBytes  70.1 Kbits/sec
[  1] 8.0000-9.0000 sec  8.55 KBytes  70.1 Kbits/sec
[  1] 9.0000-10.0000 sec  17.1 KBytes   140 Kbits/sec
[  1] 0.0000-20.3419 sec   291 KBytes   117 Kbits/sec
```

### iperf (Embedded to Host)

Linux Host:
```
$ iperf -s
```

Zephyr shell:
```
uart:~$ zperf tcp upload 192.0.2.2 5001 10 1K
Remote port is 5001
Connecting to 192.0.2.2
Duration:	10.00 s
Packet size:	1000 bytes
Rate:		10 kbps
Starting...
10.13 s
Num packets:	108
Num errors:	0 (retry or fail)
Rate:		85 Kbps
```

(Expected output on linux host after test)
```
------------------------------------------------------------
Server listening on TCP port 5001
TCP window size:  128 KByte (default)
------------------------------------------------------------
[  1] local 192.0.2.2 port 5001 connected with 192.0.2.1 port 44629
[ ID] Interval       Transfer     Bandwidth
[  1] 0.0000-10.4788 sec   105 KBytes  82.4 Kbits/sec
```
