#!/usr/bin/env python3
"""
TeslaCAN reference UART client.

Talks to the ESP32-C6 TeslaCAN firmware over the same wire protocol that
the Flipper Zero companion uses. Useful for testing the bridge without a
Flipper.

Requirements:
    pip install pyserial

Examples:
    # Watch the live event stream
    ./teslacan_client.py /dev/cu.usbserial-1234 --stream

    # One-shot status snapshot
    ./teslacan_client.py /dev/cu.usbserial-1234 --status

    # Toggle FSD off and back on
    ./teslacan_client.py /dev/cu.usbserial-1234 --fsd off
    ./teslacan_client.py /dev/cu.usbserial-1234 --fsd on
"""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial  # type: ignore[import-not-found]
except ImportError:
    sys.exit("pyserial not installed: pip install pyserial")


def open_port(path: str, baud: int) -> "serial.Serial":
    return serial.Serial(path, baud, timeout=0.5)


def send(port: "serial.Serial", line: str) -> None:
    port.write((line + "\n").encode())
    port.flush()


def read_until(port: "serial.Serial", needle: str, timeout: float) -> list[str]:
    """Collect lines until we see one that starts with `needle` or time out."""
    out: list[str] = []
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode(errors="replace").rstrip("\r\n")
        if not line:
            continue
        out.append(line)
        if line.startswith(needle):
            return out
    return out


def stream(port: "serial.Serial") -> None:
    send(port, "CMD STREAM on")
    print("# streaming — Ctrl+C to stop")
    try:
        while True:
            raw = port.readline()
            if not raw:
                continue
            line = raw.decode(errors="replace").rstrip("\r\n")
            if line:
                print(line)
    except KeyboardInterrupt:
        send(port, "CMD STREAM off")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("port", help="serial device, e.g. /dev/cu.usbserial-1234 or COM5")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--stream", action="store_true", help="continuously print events")
    ap.add_argument("--status", action="store_true", help="request one-shot status snapshot")
    ap.add_argument("--hello", action="store_true", help="handshake (prints HELLO + HW tag)")
    ap.add_argument("--fsd", choices=["on", "off"], help="toggle force-FSD")
    ap.add_argument("--mode", type=int, choices=range(5), help="set speed mode (0=Chill..4=Sloth)")
    ap.add_argument("--precond", choices=["on", "off"], help="toggle battery preconditioning")
    ap.add_argument("--isa", choices=["on", "off"], help="toggle ISA chime suppression")
    ap.add_argument("--log", choices=["on", "off"], help="toggle log mirroring")
    args = ap.parse_args()

    port = open_port(args.port, args.baud)
    time.sleep(0.2)
    port.reset_input_buffer()

    if args.hello:
        send(port, "CMD HELLO")
        for ln in read_until(port, "EVT HELLO", 2.0):
            print(ln)

    if args.fsd:
        send(port, f"CMD FSD {args.fsd}")
        for ln in read_until(port, "ACK FSD", 1.0):
            print(ln)

    if args.mode is not None:
        send(port, f"CMD MODE {args.mode}")
        for ln in read_until(port, "ACK MODE", 1.0):
            print(ln)

    if args.precond:
        send(port, f"CMD PRECOND {args.precond}")
        for ln in read_until(port, "ACK PRECOND", 1.0):
            print(ln)

    if args.isa:
        send(port, f"CMD ISA {args.isa}")
        for ln in read_until(port, "ACK ISA", 1.0):
            print(ln)

    if args.log:
        send(port, f"CMD LOG {args.log}")
        for ln in read_until(port, "ACK LOG", 1.0):
            print(ln)

    if args.status:
        send(port, "CMD STATUS")
        for ln in read_until(port, "EVT BATTERY", 2.0):
            print(ln)

    if args.stream:
        stream(port)

    return 0


if __name__ == "__main__":
    sys.exit(main())
