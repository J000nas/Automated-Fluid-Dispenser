//
// Created by Jonas Will on 08.05.25.
//

#ifndef LEDCONTROL_H
#define LEDCONTROL_H
#include <FastLED.h>
#include "config.h" 

class LedControl {
public:
    /* --Konstruktor */
    LedControl(int num);
    /* --Destruktor */
    ~LedControl();
    /* -- Beleuchtung der Leds beim Starten */
    void ledStart(const CRGB& col = CRGB::Blue);
    /* -- Methode zum setzen der Farbe für einen Platz */
    void setColor(int pos, const CRGB & col);
    /* --Methode zum blinken der LEDs */
    void blink(int interval, const CRGB & col);
    /* --Methode zum Ausschalten von allen LEDs */
    void clear();
    /* --Methode um alle LEDs gleichzeitig anzusteuern */
    void setAll(const CRGB & col);

private:
    /* --Funktion für das Blinken */
    void blinker(int interval);
    CRGB* leds;
    int _numLeds;
    bool _blinker;
    unsigned long _blinkerMillis;
};



#endif //LEDCONTROL_H
