// Klawiatura Posnet (ekrany KDF) -> klawiatura Bluetooth
//
// Klawiatura nadaje 1200 8N1, jeden bajt na nacisniecie.
// ESP32 odbiera bajt i wysyla nacisniecie klawisza przez BLE HID,
// wiec tablet widzi zwykla klawiature Bluetooth i dziala z dowolna aplikacja.
//
// Polaczenie:
//   RJ45 pin 3 (TXD)  -> MAX3232: R2I   (wejscie RS-232)
//   MAX3232: R2O      -> ESP32: GPIO4   (na DevKit V1 opisany jako D4)
//   MAX3232: VCC      -> ESP32: 3V3     (nigdy 5 V)
//   MAX3232: GND      -> ESP32: GND
//   RJ45 pin 4 (GND)  -> wspolna masa
//   RJ45 pin 1 (VCC)  -> OSOBNE zasilanie 5 V z portu USB.
//                        Zasilanie klawiatury z szyny ESP32 powoduje petle
//                        resetu przy inicjalizacji Bluetooth.
//
// UWAGA: klawiatura nadaje poziomami RS-232 (ok. +-10 V).
// Sygnal MUSI isc przez MAX3232 zasilany z 3V3. Podanie go wprost
// na GPIO — nawet przez dzielnik rezystorowy — uszkadza uklad.

#include <Arduino.h>
#include <BleKeyboard.h>
#include <esp_mac.h>

static const int PIN_RX = 4;    // na DevKit V1 opisany jako D4   // wejscie danych z klawiatury
static const int PIN_TX = 17;   // nieuzywane, nic do klawiatury nie wysylamy

// Nazwa urzadzenia BLE. Przy wielu stanowiskach KAZDE musi miec inna nazwe,
// inaczej nie odroznisz ich na liscie podczas parowania.
//
// Numer stanowiska ustaw w platformio.ini:  build_flags = -D KDS_ID=3
// Bez tego nazwa dostaje na koncu 4 znaki z adresu MAC — unikalne, ale
// nieczytelne, wiec do instalacji na stale uzywaj KDS_ID.
#ifdef KDS_ID
  #define NAZWA_BUF_INIT 1
  char nazwaBLE[32];
#else
  char nazwaBLE[32];
#endif

BleKeyboard *bleKeyboard = nullptr;

// Mapa klawiszy.
// Kod z klawiatury -> klawisz do wyslania.
// Wartosci ponizej 0x80 to znaki ASCII, powyzej to stale KEY_* z biblioteki.
// Starszy nibble = wiersz (kolejnosc od gory: E, D, B, A, C),
// mlodszy nibble: 2 = lewa kolumna, 1 = prawa.
struct KeyMapping {
  uint8_t code;
  uint8_t key;
  const char *opis;
};

static const KeyMapping KEYMAP[] = {
  // ---- nawigacja: strzalki ----
  {0xE2, KEY_LEFT_ARROW,  "lewo"},
  {0xE1, KEY_RIGHT_ARROW, "prawo"},
  {0xD2, KEY_DOWN_ARROW,  "dol"},
  {0xD1, KEY_UP_ARROW,    "gora"},

  // ---- klawisze zapasowe (fizycznie puste) ----
  {0xB2, KEY_F1, "zapasowy 1 (puste)"},
  {0xB1, KEY_F2, "zapasowy 2 (puste)"},

  // ---- funkcje KDS ----
  // Wartosci tymczasowe. Gdy poznasz skroty aplikacji KDS,
  // podmien tylko srodkowa kolumne ponizej.
  {0xA2, 'p',        "Park"},
  {0xA1, 'r',        "Recall"},
  {0xC2, 'h',        "Hold"},
  {0xC1, KEY_RETURN, "Serve / accept"},
};

static const KeyMapping *znajdz(uint8_t code) {
  for (const auto &m : KEYMAP) {
    if (m.code == code) return &m;
  }
  return nullptr;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(1200, SERIAL_8N1, PIN_RX, PIN_TX);

#ifdef KDS_ID
  snprintf(nazwaBLE, sizeof(nazwaBLE), "Klawiatura KDS %02d", (int)KDS_ID);
#else
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  snprintf(nazwaBLE, sizeof(nazwaBLE), "Klawiatura KDS %02X%02X", mac[4], mac[5]);
#endif

  bleKeyboard = new BleKeyboard(nazwaBLE, "Posnet", 100);
  bleKeyboard->begin();

  Serial.printf("\nKlawiatura Posnet -> BLE.\nSparuj urzadzenie: '%s'\n", nazwaBLE);
#ifndef KDS_ID
  Serial.println("(nazwa z adresu MAC — do instalacji na stale ustaw -D KDS_ID=n)");
#endif
}

void loop() {
  while (Serial2.available()) {
    uint8_t code = Serial2.read();
    const KeyMapping *m = znajdz(code);

    if (m == nullptr) {
      Serial.printf("nieznany kod: 0x%02X\n", code);
      continue;
    }

    Serial.printf("0x%02X  ->  %s\n", code, m->opis);

    if (bleKeyboard->isConnected()) {
      bleKeyboard->write(m->key);
    } else {
      Serial.println("  (brak polaczenia BLE - klawisz pominiety)");
    }
  }
}
