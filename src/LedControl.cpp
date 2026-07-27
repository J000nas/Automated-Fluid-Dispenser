#include "LedControl.h"

LedControl::LedControl(uint8_t num)
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
  for (uint8_t spot = 0; spot < NUM_SPOTS; spot++) {
    uint8_t baseIdx = spot * LEDS_PER_SPOT;
    for (uint8_t i = 0; i < LEDS_PER_SPOT; i++) {
      if (baseIdx + i < _numLeds) {
        leds[baseIdx + i] = col;
      }
    }
    FastLED.show();
    delay(600); // Kurze Verzögerung für den visuellen Effekt
  }

  // Am Ende der Animation alle LEDs wieder ausschalten (auf Schwarz setzen)
  for (uint8_t i = 0; i < _numLeds; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void LedControl::setColor(uint8_t pos, const CRGB &col) {
  // Da jeder Stellplatz LEDS_PER_SPOT LEDs hat (z. B. Platz 1 -> LED 0 bis 3 bei 4 LEDs/Stellplatz),
  // berechnen wir den Startindex für den LED-Streifen. pos ist 1-basiert.
  uint8_t baseIndex = LEDS_PER_SPOT * (pos - 1);

  // Validierungsprüfung, um Out-Of-Bounds-Zugriffe zu verhindern
  if (pos < 1 || pos > NUM_SPOTS || baseIndex + LEDS_PER_SPOT > _numLeds) {
    Serial.println(F("[LED] Fehler: Ungueltige Position fuer setColor."));
    return;
  }

  // Alle LEDs des Stellplatzes auf die gewünschte Farbe setzen und aktualisieren
  for (uint8_t i = 0; i < LEDS_PER_SPOT; i++) {
    leds[baseIndex + i] = col;
  }
  FastLED.show();
}

void LedControl::blink(uint16_t interval, const CRGB &col) {
  bool oldBlinker = _blinker;
  blinker(interval); // Internen Zustand der Blink-Variable aktualisieren

  // Nur ansteuern und FastLED.show() aufrufen, wenn sich der Blink-Zustand geändert hat.
  // Das verhindert ständiges Deaktivieren von Interrupts bei jedem loop()-Durchlauf,
  // was I2C-Verbindungen (MPR121) stören kann.
  if (_blinker != oldBlinker) {
    for (uint8_t i = 0; i < _numLeds; i++) {
      leds[i] = _blinker ? col : CRGB::Black;
    }
    FastLED.show();
  }
}

void LedControl::blinker(uint16_t interval) {
  unsigned long currentMillis = millis();

  // Nicht-blockierender Timer für das Umschalten des Blinkzustands (Toggle)
  if (currentMillis - _blinkerMillis >= (unsigned long)interval) {
    _blinker = !_blinker;           // Blink-Status umkehren (true <-> false)
    _blinkerMillis = currentMillis; // Zeitstempel aktualisieren
  }
}

void LedControl::clear() {
  // Alle LEDs auf Schwarz setzen (ausschalten)
  for (uint8_t i = 0; i < _numLeds; ++i) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void LedControl::setAll(const CRGB &col) {
  // Alle LEDs gleichzeitig auf dieselbe Farbe setzen
  for (uint8_t i = 0; i < _numLeds; ++i) {
    leds[i] = col;
  }
  FastLED.show();
}
