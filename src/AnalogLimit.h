#pragma once

#include "config.h"

/**
 * @class AnalogLimit
 * @brief Verwaltet die Kalibrierung und Schwellenwertermittlung der analogen
 * Becher-Erkennungssensoren.
 *
 * Diese Klasse speichert die kalibrierten analogen Grenzwerte (Schwellenwerte)
 * für jeden Stellplatz, unterhalb derer ein Becher als erkannt gilt.
 * Die Kalibrierung erfolgt durch eine Mittelwertbildung im unbeladenen Zustand.
 */
class AnalogLimit {
public:
  /**
   * @brief Konstruktor. Initialisiert alle Grenzwerte standardmäßig mit einem
   * Standardwert (888).
   */
  AnalogLimit();

  /**
   * @brief Kalibriert die Sensoren. Liest die analogen Werte der Stellplätze
   * mehrfach aus, bildet einen Mittelwert und berechnet daraus den
   * Schwellenwert (Mittelwert - Toleranzwert).
   */
  void calibrate();

  /**
   * @brief Gibt den kalibrierten Grenzwert (Schwellenwert) für einen bestimmten
   * Stellplatz zurück.
   * @param position Stellplatz-Index (0 bis NUM_SPOTS - 1)
   * @return Kalibrierter Grenzwert als Analogwert (0 bis 1023)
   */
  int getValue(int position) const;

private:
  int limits[NUM_SPOTS]; ///< Gespeicherte Schwellenwerte für die Stellplätze
};
