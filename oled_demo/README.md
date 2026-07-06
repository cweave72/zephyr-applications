# oled demo

## Build

`make build BOARD=esp32c3_042_oled/esp32c3`

## Test with Mosquitto broker

* Mosquitto config: `/etc/mosquitto/mosquitto.conf`:

```
pid_file /run/mosquitto/mosquitto.pid

persistence true
persistence_location /var/lib/mosquitto/

log_dest file /var/log/mosquitto/mosquitto.log

include_dir /etc/mosquitto/conf.d

listener 1883 0.0.0.0
allow_anonymous true
```

* Start mosquitto:
`sudo systemctl start mosquitto`

## Subscribe using mosquitto client.

* Subscribe to sensor output:
`mosquitto_sub -h localhost -t '#' -v`
