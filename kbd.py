#!/usr/bin/env python3
"""Dekoder klawiatury Posnet (KDF) na FT232R. 1200 baud, 8N1, 1 bajt = 1 klawisz."""
import os, sys, termios, select, time, glob

# Port wykrywany automatycznie. ESP32 (usbserial-0001) jest pomijany.
_porty = [p for p in sorted(glob.glob("/dev/cu.usbserial-*")) if not p.endswith("-0001")]
PORT = _porty[0] if _porty else "/dev/cu.usbserial-BRAK"
ROWS = {0xE: 1, 0xD: 2, 0xB: 3, 0xA: 4, 0xC: 5}   # starszy nibble -> wiersz (1=gora)
COLS = {2: "lewa", 1: "prawa"}                    # mlodszy nibble -> kolumna

def decode(b):
    row = ROWS.get(b >> 4)
    col = b & 0x0F
    if row is None or col not in COLS:
        return f"NIEZNANY 0x{b:02x}"
    return f"wiersz {row}/5, kolumna {COLS[col]}"

def main():
    fd = os.open(PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    a = termios.tcgetattr(fd)
    a[0] = a[1] = a[3] = 0
    a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    a[4] = a[5] = termios.B1200
    a[6][termios.VMIN] = 0; a[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, a)
    termios.tcflush(fd, termios.TCIFLUSH)
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else None
    end = time.time() + secs if secs else None
    print(f"Nasluch{f' przez {secs:.0f}s' if secs else ' — Ctrl-C konczy'}.", flush=True)
    try:
        while end is None or time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.2)
            if r:
                try: chunk = os.read(fd, 256)
                except OSError: continue
                for b in chunk:
                    print(f"0x{b:02x}  ->  {decode(b)}", flush=True)
    except KeyboardInterrupt:
        pass
    finally:
        os.close(fd)

if __name__ == "__main__":
    main()
