#pragma once

#include "config.h"
#include <Arduino.h>
#include <AccelStepper.h>

/**
 * @class StepperPump
 * @brief Steuert den Schrittmotor der Schlauchpumpe über einen TMC2209-Treiber.
 *
 * Diese Klasse kapselt die Ansteuerung des Schrittmotors für den Flüssigkeitstransport.
 * Sie implementiert:
 * - Eine kurze, knackige lineare Beschleunigungsrampe (ca. 250 ms) ohne Ruckeln oder Geschwindigkeitsabfall.
 * - Präzises, zeitgesteuertes Dosieren für automatisierte Abfüllprozesse.
 * - Kontinuierliches Fördern für manuelles Spülen / Vorpumpen via Taster.
 * - Sofortigen, abrupten Stopp der Vorwärtsförderung am Ende der Dosierzeit ohne langes Ausrollen.
 * - Einen automatischen Anti-Drip Rückzug (kurzer Rücklauf nach dem Pumpen),
 *   um Nachtropfen an der Dosierdüse zuverlässig zu verhindern.
 * - Automatisches Stromlosschalten des Treibers (TMC2209 Enable) im Ruhezustand,
 *   um Hitzeentwicklung und Stromverbrauch zu minimieren.
 *
 * WICHTIG: update() muss in jedem loop()-Durchlauf zyklisch aufgerufen werden,
 * um die Schrittimpulse und Phasen zeitgenau zu berechnen!
 */
class StepperPump {
public:
  /**
   * @brief Konstruktor. Richtet die Pins für Schrittimpuls, Richtung und Treiber-Enable ein.
   *
   * @param stepPin GPIO-Pin für das STEP-Taktsignal (Standard: PIN_PUMP_STEP aus config.h).
   * @param dirPin GPIO-Pin für das DIR-Richtungssignal (Standard: PIN_PUMP_DIR aus config.h).
   * @param enablePin GPIO-Pin für das ENABLE-Signal des TMC2209 (Active-LOW, Standard: PIN_PUMP_ENABLE).
   */
  StepperPump(uint8_t stepPin = PIN_PUMP_STEP, uint8_t dirPin = PIN_PUMP_DIR,
              uint8_t enablePin = PIN_PUMP_ENABLE);

  /**
   * @brief Initialisiert die Treiber-Hardwarepins und Parameter.
   *
   * Konfiguriert den ENABLE-Pin als Ausgang, schaltet den Treiber zunächst stromlos,
   * setzt die Maximalgeschwindigkeit sowie die minimale Impulsbreite für den TMC2209.
   * Sollte in setup() bzw. Move::begin() aufgerufen werden.
   */
  void begin();

  /**
   * @brief Führt Schrittgenerierung und Phasensteuerung aus.
   *
   * Muss zyklisch im loop() so oft wie möglich aufgerufen werden, um die
   * Schrittimpulse jitterfrei auszuführen.
   */
  void update();

  /**
   * @brief Startet einen zeitgesteuerten Dosiervorgang für die Abfüllautomatik.
   *
   * Beschleunigt kurz (250 ms) auf Maximalgeschwindigkeit, stoppt nach Ablauf der
   * Dosierzeit abrupt und führt unmittelbar einen kurzen Anti-Drip Rückzug (300 ms) durch.
   * Nach Ablauf schaltet sich der Treiber automatisch stromlos.
   *
   * @param durationMs Gesamtdauer des Pumpvorgangs in Millisekunden.
   */
  void start(uint32_t durationMs);

  /**
   * @brief Startet kontinuierliches Vorwärtspumpen (z. B. für manuelles Spülen/Vorpumpen).
   *
   * Aktiviert den Treiber und beschleunigt die Pumpe mit der kurzen Rampe
   * auf Maximalgeschwindigkeit, bis stop() oder stopWithRetract() gerufen wird.
   */
  void startContinuous();

  /**
   * @brief Bricht den Pumpvorgang sofort ab und schaltet den Treiber stromlos.
   *
   * Dient als Not-Halt und Sicherheitsabschaltung (z. B. wenn das System gestoppt wird).
   * Führt keinen Rückzug durch.
   */
  void stop();

  /**
   * @brief Stoppt die Vorwärtsförderung abrupt und führt sofort einen Anti-Drip Rückzug aus.
   *
   * Stoppt die Vorwärtsbewegung augenblicklich (kein Nachtropfen auf den Stellplatz)
   * und führt unmittelbar einen kurzen Anti-Drip Rückzug (300 ms rückwärts mit 2500 Schritte/s)
   * durch, um den Flüssigkeitstropfen sicher in den Schlauch zu saugen.
   * Schaltet den Treiber danach stromlos.
   */
  void stopWithRetract();

  /**
   * @brief Prüft, ob die Pumpe aktuell in Bewegung ist.
   *
   * @return true, solange die Pumpe aktiv fördert oder den Rückzug ausführt;
   *         false im Stillstand (IDLE).
   */
  bool isRunning() const;

  /**
   * @brief Prüft, ob der Pumpvorgang vollständig beendet ist.
   *
   * @return true, wenn die Pumpe im Ruhezustand (IDLE) steht; false solange sie läuft.
   */
  bool isFinished() const { return !isRunning(); }

private:
  // --- Feste, empirisch ermittelte Leistungsparameter ---
  static constexpr float MAX_SPEED = 10000.0f;       ///< Maximale Geschwindigkeit (Schritte/s, ca. 160 U/min)
  static constexpr float START_SPEED = 1000.0f;     ///< Anfangsgeschwindigkeit der Rampe (Schritte/s)
  static constexpr uint32_t RAMP_UP_MS = 500;       ///< Kurze Beschleunigungsrampe in ms (knackig & ohne Verzoegerung)
  static constexpr uint8_t MIN_PULSE_WIDTH = 3;     ///< Minimale Impulsbreite in µs für TMC2209
  static constexpr uint32_t RETRACT_MS = 300;       ///< Dauer des Anti-Drip Rückzugs in ms
  static constexpr float RETRACT_SPEED = 2500.0f;   ///< Rückzugsgeschwindigkeit in Schritte/s

  /**
   * @brief Interne Zustandsmaschine für die Phasensteuerung der Pumpe.
   */
  enum class State : uint8_t {
    IDLE = 0,    ///< Pumpe steht still, Treiber ist stromlos geschaltet
    FORWARD,     ///< Zeitgesteuertes Vorwärtspumpen mit kurzer Beschleunigungsrampe
    RETRACTING,  ///< Anti-Drip Rücklauf zur Tropfenvermeidung
    MANUAL       ///< Kontinuierliches Vorwärtspumpen via Taster
  };

  AccelStepper _stepper;     ///< AccelStepper-Instanz für die hardwarenahe Schrittausgabe
  uint8_t _enablePin;        ///< GPIO-Pin für das TMC2209 ENABLE-Signal

  State _state;              ///< Aktueller Zustand der State-Machine
  float _currentSpeed;       ///< Aktuelle Ist-Geschwindigkeit

  uint32_t _startTimeMs;     ///< Zeitstempel des Starts eines automatischen Dosiervorgangs
  uint32_t _durationMs;      ///< Vorgesehene Gesamtlaufzeit bei zeitgesteuerter Dosierung
  uint32_t _retractStartMs;  ///< Startzeitstempel der Rückzugsphase
  uint32_t _lastSpeedUpdateMs; ///< Zeitstempel der letzten Geschwindigkeitsaktualisierung (ms)

  /**
   * @brief Schaltet den TMC2209 Motortreiber aktiv oder stromlos.
   * @param enable true = Treiber bestromt (Pin LOW); false = stromlos/kühl (Pin HIGH).
   */
  void setDriver(bool enable);
};
