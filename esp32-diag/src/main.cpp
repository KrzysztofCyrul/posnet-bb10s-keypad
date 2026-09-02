// Mierzy najkrotszy impuls na GPIO4 = szerokosc jednego bitu.
// Z tego wprost wynika rzeczywista predkosc transmisji.

#include <Arduino.h>

static const int PIN_RX = 4;    // na DevKit V1 opisany jako D4

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RX, INPUT);
  delay(500);
  Serial.println("\n=== POMIAR SZEROKOSCI BITU ===");
  Serial.printf("Stan spoczynkowy GPIO4: %s\n",
                digitalRead(PIN_RX) ? "WYSOKI (poprawnie)" : "NISKI (problem!)");
  Serial.println("Naciskaj klawisze. Raport co 5 sekund.\n");
}

unsigned long minLow = 0xFFFFFFFF;
unsigned long minHigh = 0xFFFFFFFF;
int impulsy = 0;
unsigned long ostatniRaport = 0;

void loop() {
  unsigned long low = pulseIn(PIN_RX, LOW, 200000);
  if (low > 20) {
    if (low < minLow) minLow = low;
    impulsy++;
  }
  unsigned long high = pulseIn(PIN_RX, HIGH, 200000);
  if (high > 20 && high < 100000) {
    if (high < minHigh) minHigh = high;
  }

  if (millis() - ostatniRaport > 5000) {
    ostatniRaport = millis();
    if (impulsy == 0) {
      Serial.println("brak impulsow — linia bezczynna");
    } else {
      unsigned long bit = min(minLow, minHigh);
      Serial.printf("impulsow: %d | najkrotszy niski: %lu us | najkrotszy wysoki: %lu us\n",
                    impulsy, minLow, minHigh);
      Serial.printf("  -> szerokosc bitu ~%lu us  =  ~%lu baud\n", bit, 1000000UL / bit);
      Serial.println("  (1200 baud = 833 us, 2400 = 417 us, 9600 = 104 us)");
    }
    minLow = minHigh = 0xFFFFFFFF;
    impulsy = 0;
  }
}
