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

  // Animation: Die LEDs leuchten stellplatzweise (jeweils LEDS_PER_SPOT LEDs pro Stellplatz)
  // nacheinander auf
  for (int spot = 0; spot < NUM_SPOTS; spot++) {
    int baseIdx = spot * LEDS_PER_SPOT;
    for (int i = 0; i < LEDS_PER_SPOT; i++) {
      if (baseIdx + i < _numLeds) {
        leds[baseIdx + i] = col;
      }
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
  // Da jeder Stellplatz LEDS_PER_SPOT LEDs hat (z. B. Platz 1 -> LED 0 bis 3 bei 4 LEDs/Stellplatz),
  // berechnen wir den Startindex für den LED-Streifen. pos ist 1-basiert.
  int baseIndex = LEDS_PER_SPOT * (pos - 1);

  // Validierungsprüfung, um Out-Of-Bounds-Zugriffe zu verhindern
  if (pos < 1 || pos > NUM_SPOTS || baseIndex + LEDS_PER_SPOT > _numLeds) {
    Serial.println(F("[LED] Fehler: Ungueltige Position fuer setColor."));
    return;
  }

  // Alle LEDs des Stellplatzes auf die gewünschte Farbe setzen und aktualisieren
  for (int i = 0; i < LEDS_PER_SPOT; i++) {
    leds[baseIndex + i] = col;
  }
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
