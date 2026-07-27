#include "LedControl.h"

LedControl::LedControl(int num)
    : _numLeds(num), _blinker(false), _blinkerMillis(0) {
  leds = new CRGB[_numLeds]; // Dynamischen Speicher für die LEDs reservieren

  // FastLED initialisieren (WS2811 LED-Streifen auf dem konfigurierten Pin,
  // RGB-Farbreihenfolge)
  FastLED.addLeds<WS2811, PIN_LEDS, RGB>(leds, _numLeds);
  FastLED.setBrightness(
      100); // Standard-Helligkeit auf ca. 40% (100 von 255) setzen
}

LedControl::~LedControl() {
  delete[] leds; // Speicherlecks verhindern durch Freigabe des LED-Arrays
}

void LedControl::ledStart(const CRGB &col) {
  clear(); // Zuerst alle LEDs ausschalten

  // Animation: Die LEDs leuchten paarweise (jeweils 2 LEDs pro Stellplatz)
  // nacheinander auf
  for (int i = 0; i < _numLeds; i += 2) {
    leds[i] = col;
    if (i + 1 < _numLeds) {
      leds[i + 1] = col;
    }
    FastLED.show();
    delay(600); // Kurze Verzögerung für den visuellen Effekt
  }

  // Am Ende der Animation alle LEDs wieder ausschalten (auf Schwarz setzen)
  for (int i = 0; i < _numLeds; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void LedControl::setColor(int pos, const CRGB &col) {
  // Da jeder Stellplatz zwei LEDs hat (z. B. Platz 1 -> LED 0 und 1, Platz 2 ->
  // LED 2 und 3, etc.), berechnen wir die Indizes für den LED-Streifen. pos ist
  // 1-basiert.
  int index1 = 2 * pos - 1;
  int index2 = index1 - 1;

  // Validierungsprüfung, um Out-Of-Bounds-Zugriffe zu verhindern
  if (pos < 1 || index1 >= _numLeds || index2 < 0) {
    Serial.println(F("[LED] Fehler: Ungueltige Position fuer setColor."));
    return;
  }

  // Beide LEDs des Stellplatzes auf die gewünschte Farbe setzen und
  // aktualisieren
  leds[index1] = col;
  leds[index2] = col;
  FastLED.show();
}

void LedControl::blink(int interval, const CRGB &col) {
  blinker(interval); // Internen Zustand der Blink-Variable aktualisieren

  // Alle LEDs auf Basis des Blink-Zustands ein- oder ausschalten
  for (int i = 0; i < _numLeds; i++) {
    leds[i] = _blinker ? col : CRGB::Black;
  }
  FastLED.show();
}

void LedControl::blinker(int interval) {
  unsigned long currentMillis = millis();

  // Nicht-blockierender Timer für das Umschalten des Blinkzustands (Toggle)
  if (currentMillis - _blinkerMillis >= (unsigned long)interval) {
    _blinker = !_blinker;           // Blink-Status umkehren (true <-> false)
    _blinkerMillis = currentMillis; // Zeitstempel aktualisieren
  }
}

void LedControl::clear() {
  // Alle LEDs auf Schwarz setzen (ausschalten)
  for (int i = 0; i < _numLeds; ++i) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void LedControl::setAll(const CRGB &col) {
  // Alle LEDs gleichzeitig auf dieselbe Farbe setzen
  for (int i = 0; i < _numLeds; ++i) {
    leds[i] = col;
  }
  FastLED.show();
}
