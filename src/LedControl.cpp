#include "LedControl.h"

LedControl::LedControl(uint8_t num)
    : _numLeds(num), _blinker(false), _blinkerMillis(0), _lastUpdate(0) {
  leds = new CRGB[_numLeds]; // Dynamischen Speicher für die LEDs reservieren

  // Animationszustand für alle Stellplätze auf "aus" initialisieren
  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    _helligkeit[i] = 0;
    _zielHelligkeit[i] = 0;
    _aktuelleFarbe[i] = CRGB::Black;
    _zielFarbe[i] = CRGB::Black;
    _wellenAktiv[i] = false;
  }

  // FastLED initialisieren (WS2811 LED-Streifen auf dem konfigurierten Pin,
  // RGB-Farbreihenfolge)
  FastLED.addLeds<WS2811, PIN_LEDS, RGB>(leds, _numLeds);
  FastLED.setBrightness(
      100); // Standard-Helligkeit auf ca. 40% (100 von 255) setzen
}

LedControl::~LedControl() {
  delete[] leds; // Speicherlecks verhindern durch Freigabe des LED-Arrays
}

// ============================================================================
// update() – Muss jeden Loop aufgerufen werden!
// Berechnet Fading, Farbübergänge und Wellenanimation für alle Stellplätze.
// Ruft FastLED.show() nur auf, wenn sich tatsächlich etwas geändert hat,
// um unnötiges Deaktivieren von Interrupts zu vermeiden (I2C-Schutz).
// ============================================================================
void LedControl::update() {
  unsigned long now = millis();
  unsigned long elapsed = now - _lastUpdate;

  // Mindestens 10ms zwischen Updates warten (max. ~100 Hz)
  // Das reicht für butterweiche Animationen und schont die I2C-Kommunikation
  if (elapsed < 10) return;
  _lastUpdate = now;

  // Fade-Schritte zeitbasiert berechnen:
  // Die Originalwerte (5.5/5.0) waren für delay(5) = 5ms kalibriert.
  // Durch die Division mit 5.0 normalisieren wir auf diese Basis.
  // Beispiel: Bei 50ms Loop → fadeInStep = 5.5 * 50/5 = 55 → Fade in ~230ms
  float fadeInStep = FADE_IN_SPEED * elapsed / 5.0f;
  float fadeOutStep = FADE_OUT_SPEED * elapsed / 5.0f;
  float colorStep = COLOR_BLEND_SPEED * elapsed / 5.0f;

  bool changed = false; // Tracking, ob FastLED.show() nötig ist

  for (uint8_t spot = 0; spot < NUM_SPOTS; spot++) {
    uint8_t baseIdx = spot * LEDS_PER_SPOT;

    // --- A) Helligkeit Richtung Ziel faden ---
    if (_helligkeit[spot] < _zielHelligkeit[spot]) {
      _helligkeit[spot] += fadeInStep;
      if (_helligkeit[spot] > _zielHelligkeit[spot])
        _helligkeit[spot] = _zielHelligkeit[spot];
      changed = true;
    } else if (_helligkeit[spot] > _zielHelligkeit[spot]) {
      _helligkeit[spot] -= fadeOutStep;
      if (_helligkeit[spot] < _zielHelligkeit[spot])
        _helligkeit[spot] = _zielHelligkeit[spot];
      changed = true;
    }

    // --- B) Wenn komplett dunkel: LEDs auf Schwarz und weiter ---
    if (_helligkeit[spot] <= 0) {
      _helligkeit[spot] = 0;
      // Aktuelle Farbe zurücksetzen, damit beim nächsten Fade-In
      // die neue Farbe von Schwarz aus hochfadet
      _aktuelleFarbe[spot] = CRGB::Black;
      for (uint8_t i = 0; i < LEDS_PER_SPOT && baseIdx + i < _numLeds; i++) {
        if (leds[baseIdx + i]) { // != CRGB::Black
          leds[baseIdx + i] = CRGB::Black;
          changed = true;
        }
      }
      continue;
    }

    // --- C) Farbe sanft Richtung Zielfarbe blenden ---
    // Jede RGB-Komponente einzeln um colorStep Richtung Ziel bewegen
    changed |= blendComponent(_aktuelleFarbe[spot].r, _zielFarbe[spot].r, colorStep);
    changed |= blendComponent(_aktuelleFarbe[spot].g, _zielFarbe[spot].g, colorStep);
    changed |= blendComponent(_aktuelleFarbe[spot].b, _zielFarbe[spot].b, colorStep);

    // --- D) LEDs berechnen: Farbe × Welle × Helligkeit ---
    for (uint8_t i = 0; i < LEDS_PER_SPOT && baseIdx + i < _numLeds; i++) {
      CRGB pixelColor = _aktuelleFarbe[spot];

      // Welleneffekt: Sinuswelle rotiert über die LEDs des Stellplatzes
      if (_wellenAktiv[spot]) {
        // Jede LED bekommt einen versetzten Phasen-Offset
        uint8_t offset = i * (255 / LEDS_PER_SPOT);
        // Sinuswelle mit der Zeit rotieren lassen (millis()/4 = Drehgeschwindigkeit)
        uint8_t sinValue = sin8((uint8_t)(now / 4) + offset);
        // Welle moduliert zwischen 50% und 100% Helligkeit (nicht komplett dunkel)
        uint8_t waveVal = map(sinValue, 0, 255, 128, 255);
        pixelColor.nscale8(waveVal);
        changed = true; // Welle ändert sich ständig
      }

      // Globale Helligkeit anwenden (der Fade-Wert)
      pixelColor.nscale8((uint8_t)_helligkeit[spot]);

      leds[baseIdx + i] = pixelColor;
    }
  }

  // Nur FastLED.show() aufrufen wenn sich etwas geändert hat
  // → Minimiert Interrupt-freie Fenster → schützt I2C-Kommunikation
  if (changed) {
    FastLED.show();
  }
}

// ============================================================================
// blendComponent() – Hilfsfunktion für sanfte Farbübergänge
// Bewegt eine einzelne Farbkomponente (R/G/B) um 'step' Richtung Zielwert.
// ============================================================================
bool LedControl::blendComponent(uint8_t &current, uint8_t target, float step) {
  if (current == target) return false;

  if (current < target) {
    float newVal = current + step;
    current = (newVal >= target) ? target : (uint8_t)newVal;
  } else {
    float newVal = current - step;
    current = (newVal <= target) ? target : (uint8_t)newVal;
  }
  return true;
}

// ============================================================================
// setColor() – Setzt die Zielfarbe und startet den Fade-In
// ============================================================================
void LedControl::setColor(uint8_t pos, const CRGB &col) {
  // pos ist 1-basiert (Stellplatz 1 bis NUM_SPOTS)
  if (pos < 1 || pos > NUM_SPOTS) {
    Serial.println(F("[LED] Fehler: Ungueltige Position fuer setColor."));
    return;
  }

  uint8_t spot = pos - 1; // Auf 0-basierten Index umrechnen

  // Zielfarbe setzen – update() blendet sanft dorthin
  _zielFarbe[spot] = col;

  // Fade-In starten: Zielhelligkeit auf Maximum setzen
  _zielHelligkeit[spot] = 255;

  // Wenn der Spot gerade komplett dunkel ist (erster Fade-In),
  // aktuelle Farbe direkt auf die Zielfarbe setzen,
  // damit die Farbe sofort stimmt und nur die Helligkeit hochfadet
  if (_helligkeit[spot] <= 0) {
    _aktuelleFarbe[spot] = col;
  }
}

// ============================================================================
// clearSpot() – Startet den Fade-Out für einen Stellplatz
// ============================================================================
void LedControl::clearSpot(uint8_t pos) {
  if (pos < 1 || pos > NUM_SPOTS) return;

  uint8_t spot = pos - 1;

  // Zielhelligkeit auf 0 setzen – update() fadet sanft runter
  _zielHelligkeit[spot] = 0;

  // Wellenanimation deaktivieren (falls noch aktiv)
  _wellenAktiv[spot] = false;
}

// ============================================================================
// setWave() – Aktiviert/deaktiviert die Wellenanimation für einen Stellplatz
// ============================================================================
void LedControl::setWave(uint8_t pos, bool active) {
  if (pos < 1 || pos > NUM_SPOTS) return;
  _wellenAktiv[pos - 1] = active;
}

// ============================================================================
// ledStart() – Blockierende Start-Animation (LEDs leuchten stellplatzweise auf)
// ============================================================================
void LedControl::ledStart(const CRGB &col) {
  clear(); // Zuerst alle LEDs ausschalten

  // Animation: Die LEDs leuchten stellplatzweise (jeweils LEDS_PER_SPOT LEDs
  // pro Stellplatz) nacheinander auf
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

// ============================================================================
// blink() – Nicht-blockierendes Blinken aller LEDs (für Standby-Modus)
// ============================================================================
void LedControl::blink(uint16_t interval, const CRGB &col) {
  bool oldBlinker = _blinker;
  blinker(interval); // Internen Zustand der Blink-Variable aktualisieren

  // Nur ansteuern und FastLED.show() aufrufen, wenn sich der Blink-Zustand
  // geändert hat. Das verhindert ständiges Deaktivieren von Interrupts bei
  // jedem loop()-Durchlauf, was I2C-Verbindungen (MPR121) stören kann.
  if (_blinker != oldBlinker) {
    for (uint8_t i = 0; i < _numLeds; i++) {
      leds[i] = _blinker ? col : CRGB::Black;
    }
    FastLED.show();
  }
}

// ============================================================================
// blinker() – Timer-Logik für nicht-blockierendes Blinken
// ============================================================================
void LedControl::blinker(uint16_t interval) {
  unsigned long currentMillis = millis();

  // Nicht-blockierender Timer für das Umschalten des Blinkzustands (Toggle)
  if (currentMillis - _blinkerMillis >= (unsigned long)interval) {
    _blinker = !_blinker;           // Blink-Status umkehren (true <-> false)
    _blinkerMillis = currentMillis; // Zeitstempel aktualisieren
  }
}

// ============================================================================
// clear() – Schaltet alle LEDs sofort aus (kein Fading)
// ============================================================================
void LedControl::clear() {
  // Alle Animationszustände zurücksetzen
  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    _helligkeit[i] = 0;
    _zielHelligkeit[i] = 0;
    _aktuelleFarbe[i] = CRGB::Black;
    _zielFarbe[i] = CRGB::Black;
    _wellenAktiv[i] = false;
  }

  // Alle LEDs auf Schwarz setzen (ausschalten)
  for (uint8_t i = 0; i < _numLeds; ++i) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

// ============================================================================
// setAll() – Setzt alle LEDs sofort auf eine Farbe (kein Fading)
// ============================================================================
void LedControl::setAll(const CRGB &col) {
  // Alle LEDs gleichzeitig auf dieselbe Farbe setzen
  for (uint8_t i = 0; i < _numLeds; ++i) {
    leds[i] = col;
  }
  FastLED.show();
}
