# Klawiatura Posnet BB10S — protokół, pinout i most Bluetooth HID

Wyniki odtworzenia protokołu **klawiatury kuchennej Posnet BB10S** — 10-klawiszowej
klawiatury RS-232 używanej do obsługi ekranów KDS w gastronomii. Producent nie
publikuje dokumentacji technicznej, więc wszystko poniżej zostało **ustalone
pomiarowo na żywym sprzęcie**: prędkość transmisji, kodowanie klawiszy, pinout
złącza RJ45 i poziomy elektryczne.

W repozytorium jest też firmware dla ESP32, który zamienia tę klawiaturę
w **zwykłą klawiaturę Bluetooth**. Dzięki temu działa z dowolną aplikacją na
Androidzie — również zamkniętą, bez roota, bez modyfikowania aplikacji KDS.

---

> **English summary**
>
> Reverse-engineered protocol and wiring of the **Posnet BB10S kitchen keypad**
> (a 10-key RS-232 keypad used with Kitchen Display Systems). The vendor
> publishes no technical documentation; everything here was measured on real
> hardware.
>
> **Key findings:** 1200 baud, 8N1, one byte per keypress, no key-release code.
> Byte encoding: high nibble = row, low nibble = column; row order top to bottom
> is `E, D, B, A, C`. Signal levels are **true RS-232 (approx. ±10 V)**, not TTL,
> despite the RJ45 connector carrying VCC and GND alongside the data lines — a
> resistor divider does **not** work and will damage 3.3 V GPIO.
>
> Also included: ESP32 firmware turning the keypad into a **Bluetooth HID
> keyboard**, usable with any Android app without root.

---

## Sprzęt

| cecha | wartość |
|---|---|
| Model | Posnet BB10S, klawiatura kuchenna (KDS) |
| Klawisze | 10, układ 2 kolumny × 5 wierszy |
| Interfejs | RS-232 |
| Zasilanie | 5 V DC, 0,02 A, osobnym wtykiem USB |
| Złącze przy klawiaturze | RJ45 (8P8C) |

Producent podaje, że klawiatura nie używa zaawansowanych funkcji portu RS-232,
dzięki czemu współpracuje z konwerterami RS232-USB, i że działa na kablach do
100 metrów.

## Parametry transmisji

| parametr | wartość |
|---|---|
| Prędkość | **1200 baud** |
| Bity danych | 8 |
| Parzystość | brak |
| Bity stopu | 1 |
| Sterowanie przepływem | brak |

1200 baud to nietypowo wolno, dlatego standardowy sweep od 9600 w górę pokazuje
tylko śmieci w postaci `80 00 80 00`. Taki wzorzec — długie serie zer i
pojedyncze jedynki — zawsze oznacza, że **rzeczywista prędkość jest wolniejsza**
niż ustawiona.

Prędkość potwierdzona niezależnie pomiarem szerokości impulsu na wejściu
mikrokontrolera: **833 µs na bit**.

## Protokół

- **Jeden bajt na naciśnięcie klawisza**
- Brak kodu zwolnienia klawisza
- Brak autopowtarzania
- Klawiatura **nie nadaje nic**, dopóki nikt nie naciska

Ostatni punkt jest ważny diagnostycznie: cisza na porcie nie oznacza awarii.

Kodowanie bajtu: **starszy nibble = wiersz, młodszy nibble = kolumna.**

Kolejność wierszy od góry jest nieoczywista: **E, D, B, A, C**.

### Mapa kodów

| wiersz | kolumna lewa | kolumna prawa |
|---|---|---|
| 1 (góra) | `0xE2` | `0xE1` |
| 2 | `0xD2` | `0xD1` |
| 3 | `0xB2` | `0xB1` |
| 4 | `0xA2` | `0xA1` |
| 5 (dół) | `0xC2` | `0xC1` |

Przypisanie **lewa/prawa** opiera się na kolejności naciskania podczas
mapowania i nie zostało niezależnie potwierdzone. Jeśli w praktyce wyjdzie
odwrotnie, wystarczy zamienić `2` i `1` — struktura wierszy jest pewna.

### Etykiety funkcyjne na testowanym egzemplarzu

| kod | opis na klawiszu |
|---|---|
| `0xE2` | lewo |
| `0xE1` | prawo |
| `0xD2` | dół |
| `0xD1` | góra |
| `0xB2` | puste |
| `0xB1` | puste |
| `0xA2` | Park |
| `0xA1` | Recall |
| `0xC2` | Hold |
| `0xC1` | Serve / accept |

## Pinout złącza RJ45

Przezwoniony miernikiem. Numeracja: wtyk stykami do siebie, zatrzaskiem w dół,
pin 1 po lewej. Nie sugeruj się kolorami żył w fabrycznym kablu.

| pin | sygnał | uwagi |
|---|---|---|
| **1** | **VCC +5 V** | zasilanie klawiatury |
| 2 | RXD | wejście klawiatury, nieużywane przy samym odczycie |
| **3** | **TXD** | dane z klawiatury |
| **4** | **GND** | masa wspólna dla zasilania i sygnału |
| 5–8 | niepodłączone | |

### Sprzeczna dokumentacja krążąca w sieci

Można natrafić na opis podający `1 +5V, 3 GND, 4 RXD, 5 TXD, 6 GND, 7 +5V`.
**Przezwonienie tego nie potwierdziło.** Zgadzały się w nim wyłącznie parametry
transmisji. Powyższa tabela pochodzi z pomiaru.

### Mapowanie w fabrycznym kablu RJ45 → DB9

Fabryczny kabel Posneta jest **niestandardowy** — sygnały nie leżą tam, gdzie
oczekiwałby ich zwykły port szeregowy:

| RJ45 | DB9 |
|---|---|
| 1 (VCC) | pin 1 **oraz** +5 V wtyku USB |
| 2 (RXD) | pin 3 |
| **3 (TXD)** | **pin 4** — nie pin 2 |
| 4 (GND) | masa wtyku USB, **nie** pin 5 DB9 |

Konsekwencja: kabel **nie zadziała** z typowym modułem MAX3232 z gniazdem DB9,
bo taki moduł czyta z pinu 2 i oczekuje masy na pinie 5. Umieszczenie zasilania
na pinie 1 DB9 jest typowe dla sprzętu POS, ale niezgodne ze standardem RS-232,
gdzie pin 1 to DCD.

### Pomiń gniazdo DB9, użyj pinów kanału

Moduły MAX3232 z gniazdem DB9 są w tym zastosowaniu bezużyteczne: fabryczny
kabel ma TXD na pinie 4, a oba gniazda — kabla i modułu — są żeńskie i nie dają
się połączyć.

Rozwiązaniem jest listwa szpilkowa wyprowadzająca kanały układu:

| pin modułu | strona | rola |
|---|---|---|
| **`R2I`** | RS-232, wejście | **tu wchodzi sygnał z klawiatury** |
| **`R2O`** | TTL, wyjście | **stąd idzie do mikrokontrolera** |
| `T2I` / `T2O` | nadajnik | nieużywane przy samym odczycie |

`R` to odbiornik, `T` nadajnik, `I` wejście, `O` wyjście. Kanał 1 (`R1I`/`R1O`)
działa identycznie.

## Własny kabel RJ45

Fabryczny kabel można zastąpić zwykłym UTP zarobionym w **T568B**. Wtedy piny
odpowiadają kolorom:

| kolor żyły | pin RJ45 | sygnał |
|---|---|---|
| **biało-pomarańczowy** | 1 | VCC +5 V |
| **biało-zielony** | 3 | TXD — dane z klawiatury |
| **niebieski** | 4 | GND |

Pozostałe żyły nieużywane.

Uwaga: pin 3 i pin 4 należą do **różnych par skręconych** i nie ma na to rady,
bo pinout klawiatury jest sztywny. Przy 1200 baud nie ma to znaczenia. Na
odcinku ruchomym używaj linki, nie drutu instalacyjnego.

## Schemat połączeń z ESP32

```
KLAWIATURA
    │
    │  kabel RJ45 (T568B)
    │
    ├── BIAŁO-POMARAŃCZOWY (pin 1, +5 V) ──→ osobne zasilanie USB, patrz niżej
    │
    ├── NIEBIESKI (pin 4, GND) ────────────→ GND (wspólna masa)
    │
    └── BIAŁO-ZIELONY (pin 3, TXD) ────────→ MAX3232: R2I


MAX3232
    ├── R2O ───────────────────────────────→ ESP32: GPIO4  (na DevKit V1: D4)
    ├── VCC ───────────────────────────────→ ESP32: 3V3    (nigdy 5 V)
    └── GND ───────────────────────────────→ ESP32: GND


ESP32 ──USB──→ komputer lub ładowarka
```

### Zasilanie klawiatury musi być osobne

Klawiatura zawieszona na szynie ESP32 powoduje **pętlę resetu przy starcie
Bluetooth** — inicjalizacja radia pobiera skoki prądu rzędu 150–200 mA i
napięcie zapada. Objaw to `entry 0x400805e4` powtarzane bez końca, bez ani
jednej linii z firmware'u.

Zasil klawiaturę z osobnego portu USB. Najprościej z **drugiego portu tego
samego komputera** — wtedy masa jest wspólna automatycznie. Przy ładowarce
trzeba zewrzeć jej masę z masą ESP32.

Nigdy nie podawaj zasilania klawiatury z pinu `3V3` — stabilizator płytki tego
nie udźwignie i dioda modułu MAX3232 gaśnie.

## Firmware

### `esp32-ble-keypad/` — klawiatura Bluetooth

ESP32 zgłasza się jako klawiatura BLE HID. Tablet paruje ją jak każdą inną
klawiaturę, więc działa z dowolną aplikacją.

```bash
cd esp32-ble-keypad
pio run -t upload
python3 ../monitor.py 20      # podglad kodow, port wykrywany automatycznie
```

Mapa klawiszy jest w tablicy `KEYMAP` na górze `src/main.cpp`. Domyślnie
strzałki, Enter, F1, F2 oraz litery `p`, `r`, `h` dla Park, Recall i Hold.

Przy wielu stanowiskach **każde urządzenie musi mieć inną nazwę**, inaczej nie
odróżnisz ich na liście podczas parowania. Numer ustawia się w `platformio.ini`:

```ini
build_flags = -D KDS_ID=3     ; -> "Klawiatura KDS 03"
```

Bez tej flagi nazwa dostaje na końcu cztery znaki z adresu MAC — unikalne, ale
nieczytelne.

Po sparowaniu połączenie wraca automatycznie: BLE zapisuje powiązanie w pamięci
nieulotnej i przeżywa restart, zanik zasilania oraz wyłączenie tabletu.

### `esp32-diag/` — pomiar szerokości bitu

Mierzy najkrótszy impuls na wejściu i wprost podaje rzeczywistą prędkość
transmisji. Przydatne, gdy nie wiadomo, na jakiej prędkości pracuje urządzenie.

### `esp32-pinwatch/` — podgląd stanu wejścia na żywo

Zgłasza każdą zmianę stanu pinu. Służy do ustalenia, który pin modułu jest
wyjściem odbiornika, bez zgadywania opisów na płytce.

### `kbd.py` — odczyt na komputerze

Czysty Python, bez zależności, tylko `termios`. Wypisuje kod bajtu i pozycję
klawisza. Port wykrywany automatycznie.

```bash
python3 kbd.py        # slucha do Ctrl-C
python3 kbd.py 30     # slucha 30 sekund
```

### `monitor.py` — podgląd portu ESP32

Podgląd z prawidłowym resetem przez DTR/RTS, bez zależności zewnętrznych.
Prędkość zmienia się zmienną środowiskową `BAUD`.

## Pułapki, które kosztowały najwięcej czasu

| objaw | przyczyna |
|---|---|
| `80 00 80 00` w kółko | odczyt na zbyt wysokiej prędkości |
| bajty `0x07`, `0x0B`, `0x0F` | RS-232 podany przez dzielnik rezystorowy |
| cisza na porcie | klawiatura nadaje tylko przy naciśnięciu klawisza |
| brak impulsów przy poprawnym stanie spoczynkowym | MAX3232 z rozwartym wejściem też wystawia stan wysoki |
| gasnąca dioda modułu | zasilanie klawiatury na `3V3` albo sygnał podany na wyjście nadajnika |
| `entry 0x400805e4` w pętli | zapad napięcia przy inicjalizacji Bluetooth |
| stale wysoki stan wejścia | pływające, niepodłączone GPIO wygląda identycznie |
| `termios.error: Invalid argument` na macOS | port zablokowany po ustawieniu niestandardowej prędkości — przepnij USB |

## Ograniczenia i uwagi

- Przypisanie kolumn lewa/prawa nie zostało niezależnie zweryfikowane
- Testowane na jednym egzemplarzu klawiatury i jednym fabrycznym kablu
- Klasyczny ESP32-WROOM **nie ma USB host ani USB device**, więc wariant
  przewodowy wymaga ESP32-S3 albo innej płytki z natywnym USB
- Android bez roota nie pozwala wstrzykiwać zdarzeń klawiszy do cudzej
  aplikacji, dlatego urządzenie musi zgłaszać się jako HID, a nie jako port
  szeregowy

## Licencja

MIT — patrz [LICENSE](LICENSE).
