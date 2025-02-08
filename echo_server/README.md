# Echo Server

Demonstrates an echo server using either TCP or UDP.

## Configuration

In prj.conf set either:
```
CONFIG_ECHOSERVER_TRANSPORT_TCP=y
-or-
CONFIG_ECHOSERVER_TRANSPORT_UDP=y
```

## Building and flashing

```
make build BOARD=esp32s3_matrix/esp32s3/procpu
make flash mon
```

See boards for other supported hardware.

See `wifi_sta` app for instructions on how to set up wifi access to a AP.

## Testing

In a linux terminal (example assuming the board DHCPs 192.168.1.188):
```bash
# Using UDP:
echo "hello world" | nc -u 192.168.1.188 12001
```

Creating test files:
```
dd if=/dev/urandom of=random64.dat count=64 bs=1
dd if=/dev/urandom of=random128.dat count=128 bs=1
...
dd if=/dev/urandom of=random1k.dat count=1024 bs=1
```

Then:
```bash
cat random64.dat | nc -u 192.168.1.188 12001 | hexdump
```

>**Note**: For CONFIG_ECHOSERVER_TRANSPORT_TCP=y, simple drop the `-u` from `nc` command.
