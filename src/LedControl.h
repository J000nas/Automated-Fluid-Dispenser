#pragma once

#include "config.h"
#include <FastLED.h>

// --- Fade-Geschwindigkeiten (aus dem alten Projekt übernommen) ---
// Höhere Zahl = Schneller, Kleinere Zahl = Langsamer
// Diese Werte sind für eine Basis-Updaterate von 5ms kalibriert.
// Die update()-Methode rechnet die Geschwindigkeit automatisch zeitbasiert um,
// sodass das Fading unabhängig von der Loop-Geschwindigkeit gleich schnell läuft.
#define FADE_IN_SPEED   5.5f   // Ziemlich zügig an
#define FADE_OUT_SPEED  5.0f   // Sehr sanft aus (Soft-Off)
#define COLOR_BLEND_SPEED 5.0f // Geschwindigkeit für Farbübergänge (Rot→Gelb etc.)

/**
 * @class LedControl
 * @brief Steuert die WS2811 LED-Streifen zur optischen Visualisierung des
 * Systemstatus.
 *
 * Verwaltet die LEDs für die Stellplätze (jeweils LEDS_PER_SPOT LEDs pro Stellplatz).
 * Unterstützt sanftes Fade-In/Out, weiche Farbübergänge und eine rotierende
 * Wellenanimation während der Befüllung.
 * 
 * WICHTIG: update() muss in jedem loop()-Durchlauf aufgerufen werden,
 * damit die Animationen berechnet und die LEDs aktualisiert werden.
 */
class LedControl {
public:
  /**
   * @brief Konstruktor. Initialisiert den LED-Streifen mit FastLED und setzt
   * die Helligkeit. Alle Animationszustände werden auf Aus initialisiert.
   * @param num Anzahl der anzusteuernden LEDs insgesamt.
   */
  LedControl(uint8_t num);

  /**
   * @brief Destruktor. Gibt den dynamisch reservierten LED-Speicher frei.
   */
  ~LedControl();

  /**
   * @brief Muss jeden Loop aufgerufen werden! Berechnet Fading, Farbübergänge
   * und Wellenanimation für alle Stellplätze und ruft FastLED.show() auf,
   * wenn sich etwas geändert hat.
   * Die Fade-Geschwindigkeit ist zeitbasiert (ms), sodass sie unabhängig
   * von der Loop-Geschwindigkeit gleich schnell läuft.
   */
  void update();

  /**
   * @brief Start-Animation: LEDs leuchten nacheinander paarweise auf.
   * Blockierend (verwendet delay). Wird nur einmal beim Start aufgerufen.
   * @param col Die Farbe der Start-Animation (Standard: Blau).
   */
  void ledStart(const CRGB &col = CRGB::Blue);

  /**
   * @brief Setzt die Zielfarbe für einen Stellplatz und startet den Fade-In.
   * Wenn bereits eine andere Farbe angezeigt wird, wird sanft übergeblendet.
   * @param pos Der Stellplatz (1-basiert, 1 bis NUM_SPOTS).
   * @param col Die Zielfarbe (z. B. Rot = erkannt, Gelb = wird befüllt, Grün = fertig).
   */
  void setColor(uint8_t pos, const CRGB &col);

  /**
   * @brief Startet den Fade-Out für einen Stellplatz.
   * Die aktuelle Farbe wird sanft auf Schwarz heruntergedimmt.
   * @param pos Der Stellplatz (1-basiert, 1 bis NUM_SPOTS).
   */
  void clearSpot(uint8_t pos);

  /**
   * @brief Aktiviert oder deaktiviert die Wellenanimation für einen Stellplatz.
   * Bei aktiver Welle rotiert eine Sinuswelle über die LEDs des Stellplatzes.
   * Wird typischerweise bei Gelb (Befüllung) aktiviert und bei Grün (fertig) deaktiviert.
   * @param pos Der Stellplatz (1-basiert, 1 bis NUM_SPOTS).
   * @param active true = Welle aktivieren, false = Welle deaktivieren.
   */
  void setWave(uint8_t pos, bool active);

  /**
   * @brief Lässt alle LEDs in einer bestimmten Farbe blinken.
   * Sofortiger Effekt ohne Fading (für Standby-Modus).
   * @param interval Blink-Intervall in Millisekunden.
   * @param col Die Blinkfarbe.
   */
  void blink(uint16_t interval, const CRGB &col);

  /**
   * @brief Schaltet alle LEDs sofort aus (setzt sie auf Schwarz).
   * Kein Fading – sofortiges Ausschalten.
   */
  void clear();

  /**
   * @brief Setzt alle LEDs sofort auf eine feste Farbe.
   * Kein Fading – sofortige Anzeige.
   * @param col Die gewünschte Farbe.
   */
  void setAll(const CRGB &col);

private:
  /**
   * @brief Hilfsfunktion zur zeitlichen Steuerung des Blinkens
   * (nicht-blockierend).
   * @param interval Blink-Intervall in Millisekunden.
   */
  void blinker(uint16_t interval);

  /**
   * @brief Hilfsfunktion: Fadet eine Farbkomponente (R, G oder B) um einen
   * Schritt Richtung Zielwert.
   * @param current Aktuelle Farbkomponente (wird verändert).
   * @param target Ziel-Farbkomponente.
   * @param step Schrittweite pro Aufruf.
   * @return true wenn sich der Wert geändert hat.
   */
  bool blendComponent(uint8_t &current, uint8_t target, float step);

  CRGB *leds;    ///< Dynamisches Array für den Zustand der einzelnen LEDs
  uint8_t _numLeds;  ///< Gesamtzahl der LEDs auf dem Streifen
  bool _blinker; ///< Aktueller Blink-Zustand (an/aus)
  unsigned long _blinkerMillis; ///< Zeitstempel des letzten Zustandswechsels

  // --- Animationszustand pro Stellplatz ---
  float _helligkeit[NUM_SPOTS];      ///< Aktuelle Helligkeit (0.0 – 255.0), wird geglättet
  float _zielHelligkeit[NUM_SPOTS];  ///< Ziel-Helligkeit (255 = an, 0 = aus)
  CRGB _aktuelleFarbe[NUM_SPOTS];    ///< Aktuelle angezeigte Farbe (wird Richtung Ziel geblendet)
  CRGB _zielFarbe[NUM_SPOTS];        ///< Zielfarbe, die setColor() setzt
  bool _wellenAktiv[NUM_SPOTS];      ///< Wellenanimation aktiv (nur bei Befüllung/Gelb)

  unsigned long _lastUpdate;  ///< Zeitstempel des letzten update()-Aufrufs für zeitbasiertes Fading
};
