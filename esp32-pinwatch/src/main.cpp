// Pokazuje stan GPIO4 na zywo i zglasza kazda zmiane.
// Sluzy do sprawdzenia, ktory otwor DB9 jest wejsciem odbiornika MAX3232.
//
// Test: przewodem z pinu 3V3 dotykaj kolejnych otworow w gornym rzedzie DB9.
// Otwor, przy ktorym stan sie zmienia, jest wejsciem odbiornika.

#include <Arduino.h>

static const int PIN_RX = 4;

int poprzedni = -1;
unsigned long zmiany = 0;
unsigned long ostatniRaport = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RX, INPUT);
  delay(300);
  Serial.println("\n=== PODGLAD STANU GPIO4 ===");
  Serial.println("Dotykaj przewodem z 3V3 kolejnych otworow DB9.");
  Serial.println("Szukasz otworu, przy ktorym stan sie przelacza.\n");
}

void loop() {
  int stan = digitalRead(PIN_RX);
  if (stan != poprzedni) {
    poprzedni = stan;
    zmiany++;
    Serial.printf("[%6lu ms] stan: %s   (zmian lacznie: %lu)\n",
                  millis(), stan ? "WYSOKI" : "NISKI", zmiany);
  }
  if (millis() - ostatniRaport > 5000) {
    ostatniRaport = millis();
    Serial.printf("... stan utrzymuje sie: %s, zmian: %lu\n",
                  poprzedni ? "WYSOKI" : "NISKI", zmiany);
  }
  delay(5);
}
