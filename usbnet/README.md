# USB Network Demo

* Demonstrates ethernet over USB via CDC ECM device class.

Boards tested with:
- STM32: `nucleo_u575zi_q` (no overlay required)

Build and flash:
```
./buildall.sh --cmd=build --board=nucleo_u575zi_q
./buildall.sh --cmd=flash
```

Connect to console output:
```
sudo minicom -D /dev/serial/by-id/usb-STMicroelectronics_STLINK-<S/N>
```

Address configuration:

Device:
* MAC: 00:00:5e:00:53:00
* IP : 192.0.2.1

Host:
* MAC: 00:00:5e:00:53:01
* IP : 192.0.2.2

## Host Setup

**udev**

* Add the following rule to: `/etc/udev/rules.d/70-persistent-net.rules`

* This will detect the host address, rename the interface to `usb_zeph` and
optionally run the script.

```
ACTION=="add", SUBSYSTEM=="net", ATTR{address}=="00:00:5e:00:53:01", NAME="usb_zeph", RUN+="/opt/usb_zeph/on_connect.sh"
```

* Then:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

**netplan** (Ubuntu 22.04)

* Add the following content to `/etc/netplan/01-network-manager-all.yaml`:

```yaml
network:
  version: 2
  renderer: networkd
  ethernets:
    en1ps0:
      dhcp4: true
    # New below:
    usb_zeph:
      dhcp4: false
      addresses:
        - 192.0.2.2/24
```

Note: Using NetworkManager as the renderer causes network problems when the
usb network is connected.  Switching over to networkd as the renderer seems to
fix this.

* Then:

`sudo netplan apply`


On connect, expected dmesg:
```
[8567305.655079] cdc_ether 1-1.1:1.0 usb_zeph: unregister 'cdc_ether' usb-0000:04:00.3-1.1, CDC Ethernet Device
[8567311.200584] usb 1-1.1: new full-speed USB device number 58 using xhci_hcd
[8567311.316591] usb 1-1.1: New USB device found, idVendor=2fe3, idProduct=0100, bcdDevice= 3.07
[8567311.316606] usb 1-1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[8567311.316612] usb 1-1.1: Product: USB-DEV
[8567311.316617] usb 1-1.1: Manufacturer: ZEPHYR
[8567311.316622] usb 1-1.1: SerialNumber: 52435012002B004C
[8567311.356976] cdc_ether 1-1.1:1.0 eth0: register 'cdc_ether' at usb-0000:04:00.3-1.1, CDC Ethernet Device, 00:00:5e:00:53:01
[8567311.380343] cdc_ether 1-1.1:1.0 usb_zeph: renamed from eth0 (while UP)
```

ip addr:
```
35: usb_zeph: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 00:00:5e:00:53:01 brd ff:ff:ff:ff:ff:ff
    inet 192.0.2.2/24 brd 192.0.2.255 scope global noprefixroute usb_zeph
       valid_lft forever preferred_lft forever

```

## Testing

* Verify ping works: `ping 192.0.2.1`

* Device --> Host iperf test:

On linux host: (tested with iperf version 2.1.5)
```
iperf -s
```

On zephyr shell:
```
zperf tcp upload 192.0.2.2 5001 10 1K
```

* Host --> Device iperf test:

On zephyr shell:
```
zperf tcp download 5001
```

On linux host:
```
iperf -c 192.0.2.1
```
