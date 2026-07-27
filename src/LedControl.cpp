//
// Created by Jonas Will on 08.05.25.
//

#include "LedControl.h"

/* --Konstruktor */
LedControl::LedControl(int num): _numLeds(num), _blinker(false), _blinkerMillis(0) {
    leds = new CRGB[_numLeds];  // Speicher anlegen

    FastLED.addLeds<WS2811, PIN_LEDS, RGB>(leds, _numLeds);
    FastLED.setBrightness(100);
}

/* --Destruktor */
LedControl::~LedControl() {
    delete[] leds;
}

/* --Methode zum nacheinander aufleuchten der LEDs beim Start */
void LedControl::ledStart(const CRGB& col) {
    clear();
    for (int i =0; i<_numLeds; i+=2) {
        leds[i] = col;
        leds[i+1] = col;
        FastLED.show();
        delay(600);
    }
    for (int i=0; i<_numLeds; i++) {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
}

/* -- Methode zum Setzen der Farbe für einen Platz */
void LedControl::setColor(int pos, const CRGB &col){
    int index1 = 2 * pos - 1;
    int index2 = index1 - 1;
    if (pos < 1 || index1 >= _numLeds || index2 < 0) {
        Serial.println(F("[LED] Fehler: Ungueltige Position fuer setColor."));
        return;
    }
    leds[index1] = col;
    leds[index2] = col;
    FastLED.show();
}

/* --Methode zum blinken der LEDs */
void LedControl::blink(int interval, const CRGB &col) {
    blinker(interval); // aktualisiert den internen Zustand (_blinker)

    for (int i = 0; i < _numLeds; i++) {
        leds[i] = _blinker ? col : CRGB::Black;
    }
    FastLED.show();
}

/* --Funktion für das Blinken */
void LedControl::blinker(int interval) {
    unsigned long currentMillis = millis();

    if (currentMillis - _blinkerMillis >= interval) {
        _blinker = !_blinker; // Status wechseln
        _blinkerMillis = currentMillis;
    }
}

/* --Methode zum Ausschalten von allen LEDs */
void LedControl::clear() {
    for (int i = 0; i < _numLeds; ++i) {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
}

/* --Methode um alle LEDs gleichzeitig anzusteuern */
void LedControl::setAll(const CRGB &col) {
    for (int i = 0; i < _numLeds; ++i) {
        leds[i] = col;
    }
    FastLED.show();
}

