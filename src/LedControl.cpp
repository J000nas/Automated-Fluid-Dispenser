#include "LedControl.h"

LedControl::LedControl(uint8_t num)
    : _numLeds(num), _blinker(false), _blinkerMillis(0), _lastUpdate(0), _standbyStart(0) {
  leds = new CRGB[_numLeds]; // Dynamischen Speicher für die LEDs reservieren

  // Animationszustand für alle Stellplätze auf "aus" initialisieren
  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    _helligkeit[i] = 0;
    _zielHelligkeit[i] = 0;
    _aktuelleFarbeR[i] = 0;
    _aktuelleFarbeG[i] = 0;
    _aktuelleFarbeB[i] = 0;
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
// Ruft FastLED.show() nur auf, wenn sich tatsächlich etwas geändert hat.
// ============================================================================
void LedControl::update() {
  unsigned long now = millis();
  unsigned long elapsed = now - _lastUpdate;

  // Mindestens 10ms zwischen Updates warten (max. ~100 Hz)
  if (elapsed < 10) return;
  _lastUpdate = now;

  // Fade-Schritte zeitbasiert berechnen:
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
      _aktuelleFarbeR[spot] = 0;
      _aktuelleFarbeG[spot] = 0;
      _aktuelleFarbeB[spot] = 0;
      for (uint8_t i = 0; i < LEDS_PER_SPOT && baseIdx + i < _numLeds; i++) {
        if (leds[baseIdx + i]) { // != CRGB::Black
          leds[baseIdx + i] = CRGB::Black;
          changed = true;
        }
      }
      continue;
    }

    // --- C) Farbe sanft Richtung Zielfarbe blenden ---
    float oldR = _aktuelleFarbeR[spot];
    float oldG = _aktuelleFarbeG[spot];
    float oldB = _aktuelleFarbeB[spot];

    _aktuelleFarbeR[spot] = blendValue(_aktuelleFarbeR[spot], _zielFarbe[spot].r, colorStep);
    _aktuelleFarbeG[spot] = blendValue(_aktuelleFarbeG[spot], _zielFarbe[spot].g, colorStep);
    _aktuelleFarbeB[spot] = blendValue(_aktuelleFarbeB[spot], _zielFarbe[spot].b, colorStep);

    if (_aktuelleFarbeR[spot] != oldR || _aktuelleFarbeG[spot] != oldG || _aktuelleFarbeB[spot] != oldB) {
      changed = true;
    }

    // --- D) LEDs berechnen: Farbe × Welle × Helligkeit ---
    for (uint8_t i = 0; i < LEDS_PER_SPOT && baseIdx + i < _numLeds; i++) {
      CRGB pixelColor = CRGB((uint8_t)_aktuelleFarbeR[spot], (uint8_t)_aktuelleFarbeG[spot], (uint8_t)_aktuelleFarbeB[spot]);

      // Welleneffekt: Sinuswelle rotiert über die LEDs des Stellplatzes
      if (_wellenAktiv[spot]) {
        // Jede LED bekommt einen versetzten Phasen-Offset
        uint8_t offset = i * (255 / LEDS_PER_SPOT);
        // Sinuswelle rotieren lassen (now/3 für eine harmonischere, flüssige Bewegung)
        uint8_t sinValue = sin8((uint8_t)(now / 3) + offset);
        // Welle moduliert deutlicher zwischen 20% und 100% Helligkeit (höherer Kontrast für bessere Sichtbarkeit)
        uint8_t waveVal = map(sinValue, 0, 255, 50, 255);
        pixelColor.nscale8(waveVal);
        changed = true; // Welle ist immer in Bewegung
      }

      // Globale Helligkeit anwenden (der Fade-Wert)
      pixelColor.nscale8((uint8_t)_helligkeit[spot]);

      leds[baseIdx + i] = pixelColor;
    }
  }

  if (changed) {
    FastLED.show();
  }
}

// ============================================================================
// blendValue() – Hilfsfunktion für float-Fading
// ============================================================================
float LedControl::blendValue(float current, float target, float step) {
  if (current == target) return current;

  if (current < target) {
    current += step;
    if (current > target) current = target;
  } else {
    current -= step;
    if (current < target) current = target;
  }
  return current;
}

// ============================================================================
// setColor() – Setzt die Zielfarbe und startet den Fade-In
// ============================================================================
void LedControl::setColor(uint8_t pos, const CRGB &col) {
  if (pos < 1 || pos > NUM_SPOTS) {
    Serial.println(F("[LED] Fehler: Ungueltige Position fuer setColor."));
    return;
  }

  uint8_t spot = pos - 1;

  _zielFarbe[spot] = col;
  _zielHelligkeit[spot] = 255;

  // Wenn der Spot komplett dunkel ist, Farbe direkt setzen (ohne Farbmischungs-Übergang von Schwarz)
  if (_helligkeit[spot] <= 0) {
    _aktuelleFarbeR[spot] = col.r;
    _aktuelleFarbeG[spot] = col.g;
    _aktuelleFarbeB[spot] = col.b;
  }

  _standbyStart = 0; // Standby-Zeit zurücksetzen, da wir wieder im aktiven Modus sind
}

// ============================================================================
// clearSpot() – Startet den Fade-Out für einen Stellplatz
// ============================================================================
void LedControl::clearSpot(uint8_t pos) {
  if (pos < 1 || pos > NUM_SPOTS) return;
  uint8_t spot = pos - 1;
  _zielHelligkeit[spot] = 0;
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

  // 1. Aktivierungs-Spin für jeden Ring nacheinander
  for (uint8_t spot = 0; spot < NUM_SPOTS; spot++) {
    uint8_t baseIdx = spot * LEDS_PER_SPOT;
    
    for (int round = 0; round < 2; round++) {
      for (int i = 0; i < LEDS_PER_SPOT; i++) {
        for (int k = 0; k < LEDS_PER_SPOT; k++) {
          leds[baseIdx + k].fadeToBlackBy(80);
        }
        
        uint8_t hue = 160 - (spot * 12) - (i * 10);
        leds[baseIdx + i] = CHSV(hue, 255, 255);
        
        FastLED.show();
        delay(35);
      }
    }
    
    for (int k = 0; k < LEDS_PER_SPOT; k++) {
      leds[baseIdx + k] = col;
      leds[baseIdx + k].nscale8(40);
    }
    FastLED.show();
  }

  delay(200);

  // 2. Synchroner Atem-Effekt (Breathing Pulse) aller Ringe
  for (int b = 40; b <= 255; b += 6) {
    for (int spot = 0; spot < NUM_SPOTS; spot++) {
      uint8_t baseIdx = spot * LEDS_PER_SPOT;
      for (int k = 0; k < LEDS_PER_SPOT; k++) {
        CRGB tempColor = col;
        tempColor.nscale8(b);
        leds[baseIdx + k] = tempColor;
      }
    }
    FastLED.show();
    delay(10);
  }
  
  delay(400);

  for (int b = 255; b >= 30; b -= 6) {
    for (int spot = 0; spot < NUM_SPOTS; spot++) {
      uint8_t baseIdx = spot * LEDS_PER_SPOT;
      for (int k = 0; k < LEDS_PER_SPOT; k++) {
        CRGB tempColor = col;
        tempColor.nscale8(b);
        leds[baseIdx + k] = tempColor;
      }
    }
    FastLED.show();
    delay(10);
  }

  // WICHTIG: Kein clear() aufrufen! Die LEDs bleiben bei Helligkeit 30 (Dunkelblau) stehen.
  // _standbyStart wird auf 0 gesetzt, damit showStandbyPulse() nahtlos bei Helligkeit 30 übernimmt.
  _standbyStart = 0;
}

// ============================================================================
// blink() – Nicht-blockierendes Blinken aller LEDs (für Standby-Modus)
// ============================================================================
void LedControl::blink(uint16_t interval, const CRGB &col) {
  bool oldBlinker = _blinker;
  blinker(interval);

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
  if (currentMillis - _blinkerMillis >= (unsigned long)interval) {
    _blinker = !_blinker;
    _blinkerMillis = currentMillis;
  }
}

// ============================================================================
// showStandbyPulse() – Lässt alle LEDs im Standby weich pulsieren (Blau)
// ============================================================================
void LedControl::showStandbyPulse(const CRGB &col) {
  // Wenn wir frisch in den Standby kommen, Startzeitpunkt merken
  if (_standbyStart == 0) {
    _standbyStart = millis();
  }

  unsigned long t = millis() - _standbyStart;

  // Weicher sinusförmiger Atem-Effekt
  // +192 sorgt dafür, dass der Sinus genau bei seinem Minimum (0) startet.
  // Dadurch ist der Übergang von ledStart() (das bei Helligkeit 30 endet)
  // zu showStandbyPulse() (das bei Helligkeit 30 startet) absolut nahtlos und stufenlos!
  uint8_t pulse = ease8InOutApprox(sin8((t / 10) + 192));
  
  // Helligkeit moduliert zwischen 30 (ca. 12%) und 140 (ca. 55%)
  uint8_t brightness = map(pulse, 0, 255, 30, 140);
  
  for (uint8_t i = 0; i < _numLeds; i++) {
    CRGB tempColor = col;
    tempColor.nscale8(brightness);
    leds[i] = tempColor;
  }
  FastLED.show();
}

// ============================================================================
// clear() – Schaltet alle LEDs sofort aus (kein Fading)
// ============================================================================
void LedControl::clear() {
  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    _helligkeit[i] = 0;
    _zielHelligkeit[i] = 0;
    _aktuelleFarbeR[i] = 0;
    _aktuelleFarbeG[i] = 0;
    _aktuelleFarbeB[i] = 0;
    _zielFarbe[i] = CRGB::Black;
    _wellenAktiv[i] = false;
  }

  _standbyStart = 0; // Standby-Zeit zurücksetzen

  for (uint8_t i = 0; i < _numLeds; ++i) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

// ============================================================================
// setAll() – Setzt alle LEDs sofort auf eine Farbe (kein Fading)
// ============================================================================
void LedControl::setAll(const CRGB &col) {
  for (uint8_t i = 0; i < _numLeds; ++i) {
    leds[i] = col;
  }
  FastLED.show();
}
