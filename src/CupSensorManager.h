#pragma once
#include <Adafruit_MPR121.h>
#include <Arduino.h>

/**
 * @class CupSensorManager
 * @brief Verwaltet die kapazitive Glaserkennung von 5 Stellplätzen über den
 * MPR121.
 *
 * Beinhaltet:
 * - Manuelle Kalibrierung für C = 12.4pF (keine Auto-Config Fehlschläge)
 * - Schnelles Baseline-Drift-Tracking beim Entfernen
 * - Gesperrtes Drift-Tracking bei stehendem Glas (unbegrenzte Erkennungsdauer)
 * - Asymmetrisches Software-Crosstalk-Kompensationsmodell (55% S1->S0, 24%
 * sonstige)
 * - Hysterese (Touch = 8, Release = 6)
 */
class CupSensorManager {
public:
  static const uint8_t NUM_SENSORS = 5;

  CupSensorManager();

  /**
   * @brief Initialisiert den MPR121-Sensor und konfiguriert die
   * Tuning-Register.
   * @param i2cAddress Die I2C-Adresse des MPR121 (Standard: 0x5A)
   * @return true wenn erfolgreich initialisiert, false bei Fehler
   */
  bool begin(uint8_t i2cAddress = 0x5A);

  /**
   * @brief Aktualisiert die Sensordaten. Muss regelmäßig im loop() aufgerufen
   * werden.
   */
  void update();

  /**
   * @brief Prüft, ob auf einem bestimmten Stellplatz ein Glas steht.
   * @param index Stellplatz-Index (0 bis 4)
   * @return true wenn Glas erkannt, false wenn leer
   */
  bool isCupDetected(uint8_t index) const;

  /**
   * @brief Gibt den crosstalk-kompensierten Differenzwert zurück.
   * @param index Stellplatz-Index (0 bis 4)
   * @return Kompensierter Wert (0 bis ca. 30+)
   */
  int16_t getCompensatedDiff(uint8_t index) const;

  /**
   * @brief Gibt den rohen Differenzwert zurück (vor der
   * Crosstalk-Kompensation).
   * @param index Stellplatz-Index (0 bis 4)
   * @return Rausgeschnittener Raw-Differenzwert
   */
  int16_t getRawDiff(uint8_t index) const;

  /**
   * @brief Gibt den physischen MPR121-Pin für einen Stellplatz zurück.
   * @param index Stellplatz-Index (0 bis 4)
   * @return Physischer Pin (0 bis 4)
   */
  uint8_t getSensorPin(uint8_t index) const;

private:
  Adafruit_MPR121 cap;
  const uint8_t SENSOR_PINS[NUM_SENSORS] = {0, 1, 2, 3, 4};

  // Hysterese-Schwellenwerte
  const uint8_t TOUCH_THRESHOLD = 8;
  const uint8_t RELEASE_THRESHOLD = 6;

  // Zustandsvariablen
  bool glasErkannt[NUM_SENSORS];
  int16_t differenzenRaw[NUM_SENSORS];
  int16_t differenzenKompensiert[NUM_SENSORS];

  // Hilfsfunktion zum direkten Schreiben von I2C-Registern
  void writeRegister(uint8_t reg, uint8_t value);
};
