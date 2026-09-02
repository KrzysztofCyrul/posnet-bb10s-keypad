#!/usr/bin/env python3
"""Podglad portu szeregowego ESP32 z poprawnym resetem przez DTR/RTS.
Uzycie: python3 monitor.py [sekundy]   (domyslnie 20)
Dziala bez zadnych zaleznosci - czysty termios."""

import os, sys, termios, fcntl, struct, select, time, glob

WZORCE = ["/dev/cu.usbserial-*", "/dev/cu.SLAB_USBtoUART*", "/dev/cu.wchusbserial*"]
TIOCM_DTR = getattr(termios, "TIOCM_DTR", 0x002)
TIOCM_RTS = getattr(termios, "TIOCM_RTS", 0x004)
TIOCMBIS  = getattr(termios, "TIOCMBIS", 0x8004746c)
TIOCMBIC  = getattr(termios, "TIOCMBIC", 0x8004746b)

porty = sorted(p for w in WZORCE for p in glob.glob(w))
if not porty:
    print("brak portu szeregowego — czy ESP32 jest podpiete?")
    sys.exit(1)
PORT = sys.argv[2] if len(sys.argv) > 2 else porty[0]
secs = float(sys.argv[1]) if len(sys.argv) > 1 else 20
BAUD = int(os.environ.get("BAUD", "115200"))

try:
    fd = os.open(PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
except OSError as e:
    print(f"nie moge otworzyc {PORT}: {e}")
    print("jesli 'Resource busy' — zamknij monitor w Arduino IDE albo pio device monitor")
    sys.exit(1)

a = termios.tcgetattr(fd)
a[0] = a[1] = a[3] = 0
a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
a[4] = a[5] = termios.B9600          # placeholder, prawdziwa predkosc ponizej
a[6][termios.VMIN] = 0
a[6][termios.VTIME] = 0
termios.tcsetattr(fd, termios.TCSANOW, a)

# IOSSIOSPEED — jedyna metoda na macOS, ktora dziala takze dla niestandardowych
# predkosci i nie zostawia sterownika w stanie odrzucajacym kolejne wywolania.
IOSSIOSPEED = 0x80045402
fcntl.ioctl(fd, IOSSIOSPEED, struct.pack("I", BAUD))

def linia(bits, on):
    fcntl.ioctl(fd, TIOCMBIS if on else TIOCMBIC, struct.pack("I", bits))

linia(TIOCM_DTR, False)   # GPIO0 wysoki = praca normalna
linia(TIOCM_RTS, True)    # EN niski = reset
time.sleep(0.15)
linia(TIOCM_RTS, False)   # zwolnij reset
termios.tcflush(fd, termios.TCIFLUSH)

print(f"--- {PORT} @ {BAUD}, {secs:.0f}s. NACISKAJ KLAWISZE ---", flush=True)
end = time.time() + secs
buf = b""
n = 0
while time.time() < end:
    r, _, _ = select.select([fd], [], [], 0.2)
    if r:
        try:
            buf += os.read(fd, 4096)
        except OSError:
            pass
        while b"\n" in buf and n < 80:
            line, buf = buf.split(b"\n", 1)
            print("  " + line.decode("utf-8", "replace").rstrip(), flush=True)
            n += 1
os.close(fd)
print(f"--- koniec, linii: {n} ---")
