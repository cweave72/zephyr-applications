#!/usr/bin/env python3
"""End-to-end test for the net_serial_bridge app.

Drives the bridged serial port and the TCP port at the same time and checks
that bytes cross in both directions unchanged. Uses only the standard library
so it runs from python/.venv without extra packages.

Usage:
    python test/bridge_test.py [--serial DEV] [--ip ADDR] [--port N]
                               [--data FILE] [--timeout S]
"""

import argparse
import os
import select
import socket
import sys
import termios
import threading
import time

PROBE_SERIAL = (
    "/dev/serial/by-id/"
    "usb-Raspberry_Pi_Debug_Probe__CMSIS-DAP__E6647C74037E882F-if01"
)
WORKSPACE_RANDOM = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "random1k.dat"
)


class Serial:
    """Raw serial port at a fixed baud rate."""

    def __init__(self, dev, baud=termios.B115200):
        self.fd = os.open(dev, os.O_RDWR | os.O_NOCTTY)
        attrs = termios.tcgetattr(self.fd)
        attrs[0] = 0                                        # iflag
        attrs[1] = 0                                        # oflag
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # cflag
        attrs[3] = 0                                        # lflag
        attrs[4] = baud                                     # ispeed
        attrs[5] = baud                                     # ospeed
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        self.flush()

    def flush(self):
        termios.tcflush(self.fd, termios.TCIOFLUSH)

    def write(self, data):
        view = memoryview(data)
        while view:
            num = os.write(self.fd, view)
            view = view[num:]
        termios.tcdrain(self.fd)

    def read(self, count, timeout):
        """Read count bytes or stop at timeout. Returns what arrived."""
        buf = bytearray()
        deadline = time.monotonic() + timeout
        while len(buf) < count:
            remain = deadline - time.monotonic()
            if remain <= 0:
                break
            ready, _, _ = select.select([self.fd], [], [], remain)
            if ready:
                buf += os.read(self.fd, count - len(buf))
        return bytes(buf)

    def drain(self, quiet=0.2):
        """Discard bytes until the line is quiet."""
        while self.read(4096, quiet):
            pass

    def close(self):
        os.close(self.fd)


class Tcp:
    """Blocking TCP client with a per-read deadline."""

    def __init__(self, ip, port, timeout):
        self.sock = socket.create_connection((ip, port), timeout=5)
        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.timeout = timeout

    def write(self, data):
        self.sock.sendall(data)

    def read(self, count, timeout=None):
        """Read count bytes or stop at timeout. Returns what arrived."""
        buf = bytearray()
        deadline = time.monotonic() + (timeout or self.timeout)
        while len(buf) < count:
            remain = deadline - time.monotonic()
            if remain <= 0:
                break
            self.sock.settimeout(remain)
            try:
                chunk = self.sock.recv(count - len(buf))
            except socket.timeout:
                break
            if not chunk:
                break
            buf += chunk
        return bytes(buf)

    def drain(self, quiet=0.2):
        while self.read(4096, quiet):
            pass

    def close(self):
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self.sock.close()


class Report:
    def __init__(self):
        self.failures = 0

    def result(self, name, ok, detail):
        tag = "PASS" if ok else "FAIL"
        if not ok:
            self.failures += 1
        print(f"[{tag}] {name}: {detail}")


def first_mismatch(sent, got):
    for i, (a, b) in enumerate(zip(sent, got)):
        if a != b:
            return i
    return min(len(sent), len(got))


def check_equal(report, name, sent, got, elapsed):
    ok = sent == got
    rate = (len(got) * 8 / elapsed / 1000) if elapsed > 0 else 0
    detail = f"{len(got)}/{len(sent)} bytes in {elapsed:.3f}s ({rate:.1f} kbit/s)"
    if not ok:
        detail += f", first mismatch at offset {first_mismatch(sent, got)}"
    report.result(name, ok, detail)


def test_serial_to_tcp(report, ser, tcp, data, timeout, name="serial->tcp"):
    start = time.monotonic()
    ser.write(data)
    got = tcp.read(len(data), timeout)
    check_equal(report, name, data, got, time.monotonic() - start)


def test_tcp_to_serial(report, ser, tcp, data, timeout, name="tcp->serial"):
    start = time.monotonic()
    tcp.write(data)
    got = ser.read(len(data), timeout)
    check_equal(report, name, data, got, time.monotonic() - start)


def test_full_duplex(report, ser, tcp, data, timeout):
    """Both directions at once. Readers start first so nothing backs up."""
    s2t_got = []
    t2s_got = []

    def read_tcp():
        s2t_got.append(tcp.read(len(data), timeout))

    def read_serial():
        t2s_got.append(ser.read(len(data), timeout))

    readers = [threading.Thread(target=read_tcp),
               threading.Thread(target=read_serial)]
    writers = [threading.Thread(target=ser.write, args=(data,)),
               threading.Thread(target=tcp.write, args=(data,))]
    start = time.monotonic()
    for t in readers + writers:
        t.start()
    for t in readers + writers:
        t.join()
    elapsed = time.monotonic() - start
    check_equal(report, "duplex serial->tcp", data, s2t_got[0], elapsed)
    check_equal(report, "duplex tcp->serial", data, t2s_got[0], elapsed)


def test_buffered_while_disconnected(report, ser, args, data, pool_bytes):
    """Serial bytes sent with no client are held, oldest dropped first."""
    # Small burst: everything fits and arrives on connect.
    ser.write(data)
    time.sleep(0.5)
    tcp = Tcp(args.ip, args.port, args.timeout)
    start = time.monotonic()
    got = tcp.read(len(data), args.timeout)
    check_equal(report, "held while disconnected", data, got,
                time.monotonic() - start)
    tcp.close()
    time.sleep(0.3)

    # Large burst: twice the pool. Expect a suffix of at most pool_bytes.
    big = data * max(2, (2 * pool_bytes) // len(data))
    ser.write(big)
    time.sleep(0.5)
    tcp = Tcp(args.ip, args.port, args.timeout)
    got = tcp.read(len(big), 1.0)
    ok = 0 < len(got) <= pool_bytes and big.endswith(got)
    report.result(
        "drop oldest",
        ok,
        f"sent {len(big)}, kept {len(got)} (pool {pool_bytes}), "
        f"suffix={'yes' if big.endswith(got) else 'no'}",
    )
    return tcp


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--serial", default=PROBE_SERIAL)
    ap.add_argument("--ip", default="192.168.1.15")
    ap.add_argument("--port", type=int, default=12001)
    ap.add_argument("--data", default=WORKSPACE_RANDOM)
    ap.add_argument("--timeout", type=float, default=3.0)
    ap.add_argument("--pool-bytes", type=int, default=16 * 256,
                    help="APP_BRIDGE_POOL_DEPTH * APP_BRIDGE_ITEM_SIZE "
                         "(16*256 for uart0, 64*256 for cdc_acm)")
    args = ap.parse_args()

    with open(args.data, "rb") as f:
        data = f.read()
    print(f"serial={args.serial} tcp={args.ip}:{args.port} "
          f"data={len(data)} bytes")

    report = Report()
    ser = Serial(args.serial)
    tcp = Tcp(args.ip, args.port, args.timeout)

    # Discard anything left over from a previous run or from boot.
    ser.drain()
    tcp.drain()

    test_serial_to_tcp(report, ser, tcp, data, args.timeout)
    test_tcp_to_serial(report, ser, tcp, data * 2, args.timeout)
    test_full_duplex(report, ser, tcp, data * 4, args.timeout * 2)

    tcp.close()
    time.sleep(0.3)
    tcp = test_buffered_while_disconnected(report, ser, args, data,
                                           args.pool_bytes)

    tcp.close()
    time.sleep(0.3)
    tcp = Tcp(args.ip, args.port, args.timeout)
    ser.drain()
    tcp.drain()
    test_serial_to_tcp(report, ser, tcp, data, args.timeout, name="reconnect")

    tcp.close()
    ser.close()

    print(f"{report.failures} failure(s)")
    return 1 if report.failures else 0


if __name__ == "__main__":
    sys.exit(main())
