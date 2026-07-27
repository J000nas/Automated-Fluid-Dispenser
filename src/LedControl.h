#pragma once

#include "config.h"
#include <FastLED.h>

/**
 * @class LedControl
 * @brief Steuert die WS2811 LED-Streifen zur optischen Visualisierung des
 * Systemstatus.
 *
 * Verwaltet die LEDs für die Stellplätze (jeweils 2 LEDs pro Stellplatz) sowie
 * Start- und Statuseffekte. Unterstützt das Blinken einzelner/aller LEDs, das
 * Setzen spezifischer Farben pro Platz und Start-Animationen.
 */
class LedControl {
public:
  /**
   * @brief Konstruktor. Initialisiert den LED-Streifen mit FastLED und setzt
   * die Helligkeit.
   * @param num Anzahl der anzusteuernden LEDs insgesamt.
   */
  LedControl(int num);

  /**
   * @brief Destruktor. Gibt den dynamisch reservierten LED-Speicher frei.
   */
  ~LedControl();

  /**
   * @brief Start-Animation: LEDs leuchten nacheinander paarweise auf.
   * @param col Die Farbe der Start-Animation (Standard: Blau).
   */
  void ledStart(const CRGB &col = CRGB::Blue);

  /**
   * @brief Setzt die Farbe für ein bestimmtes Glas/Stellplatz.
   * @param pos Der Stellplatz (1-basiert, 1 bis NUM_SPOTS).
   * @param col Die anzuzeigende Farbe (z. B. Rot = Becher erkannt, Gelb = Wird
   * befüllt, Grün = Fertig).
   */
  void setColor(int pos, const CRGB &col);

  /**
   * @brief Lässt alle LEDs in einer bestimmten Farbe blinken.
   * @param interval Blink-Intervall in Millisekunden.
   * @param col Die Blinkfarbe.
   */
  void blink(int interval, const CRGB &col);

  /**
   * @brief Schaltet alle LEDs aus (setzt sie auf Schwarz).
   */
  void clear();

  /**
   * @brief Setzt alle LEDs gleichzeitig auf eine feste Farbe.
   * @param col Die gewünschte Farbe.
   */
  void setAll(const CRGB &col);

private:
  /**
   * @brief Hilfsfunktion zur zeitlichen Steuerung des Blinkens
   * (nicht-blockierend).
   * @param interval Blink-Intervall in Millisekunden.
   */
  void blinker(int interval);

  CRGB *leds;    ///< Dynamisches Array für den Zustand der einzelnen LEDs
  int _numLeds;  ///< Gesamtzahl der LEDs auf dem Streifen
  bool _blinker; ///< Aktueller Blink-Zustand (an/aus)
  unsigned long _blinkerMillis; ///< Zeitstempel des letzten Zustandswechsels
                                ///< für das Blinken
};
