#pragma once

#include "CupSensorManager.h"
#include "LedControl.h"
#include "Queue.h"
#include "config.h"
#include <Arduino.h>
#include <MobaTools.h>

/**
 * @class Move
 * @brief Steuert den Servo-Motor, die Befüllungspumpe und liest den Status der
 * Bechersensoren aus.
 *
 * Diese Klasse implementiert die Haupt-Zustandsmaschine (State Machine) für den
 * Bewegungs- und Abfüllprozess. Sie fragt die Stellplatzsensoren ab, fügt
 * belegte Plätze der Warteschlange (Queue) hinzu und steuert den Servo-Arm zur
 * Abfüllung an.
 */
class Move {
public:
  /**
   * @brief Konstruktor. Richtet die Sensor-Pins mit internem Pullup-Widerstand
   * ein.
   */
  Move();

  /**
   * @brief Fragt zyklisch die analogen Sensoren der Stellplätze ab und
   * aktualisiert die Warteschlange.
   *
   * Prüft, ob ein Glas platziert oder entfernt wurde (mit Entprellung von 200
   * ms). Ändert bei Erkennung die LED-Farbe des Stellplatzes (Rot = Becher
   * erkannt).
   *
   * @param queue Die Warteschlange, in die Stellplatzpositionen
   * eingetragen/gelöscht werden.
   * @param led Referenz zur LED-Steuerung zur Anzeige des Stellplatz-Status.
   * @param sensorManager Referenz zum kapazitiven CupSensorManager.
   */
  void status(Queue &queue, LedControl &led, CupSensorManager &sensorManager);

  /**
   * @brief Ermöglicht das manuelle Vorpumpen/Spülen über einen Taster.
   *
   * Liest den Zustand von `PIN_PUMP_TASTER` ein, entprellt diesen und schaltet
   * das Pumpenrelais entsprechend direkt an oder aus.
   */
  void pump();

  /**
   * @brief Initialisiert die Hardware-Pins für das Pumpenrelais und den
   * manuellen Pumpentaster.
   */
  void begin();

  /**
   * @brief Führt die Zustandsmaschine zur Ablaufsteuerung der Abfüllung aus.
   *
   * Beinhaltet folgende Phasen:
   * - IDLE: Wartet auf Einträge in der Queue oder fährt zurück zur
   * Nullposition.
   * - MOVING: Wartet, bis der Servo die Zielposition erreicht hat.
   * - WAITING_PUMP: Aktiviert die Pumpe für eine definierte Zeit
   * (WAIT_TIME_PUMP).
   * - WAITING_NEXT: Hält den Servo nach dem Pumpen noch kurz an
   * (Gesamtwartezeit WAIT_TIME) und löscht danach die Position aus der Queue.
   *
   * @param queue Referenz auf die Positions-Warteschlange.
   * @param led Referenz auf die LED-Steuerung (ändert Farben während der
   * Zustandswechsel).
   */
  void run(Queue &queue, LedControl &led);

  /**
   * @brief Fährt den Servo in die Park- bzw. Nullposition (0 Grad).
   */
  void toZero();

  /**
   * @brief Koppelt den Servo an den angegebenen Steuerungs-Pin an und setzt die
   * Stellgeschwindigkeit.
   * @param pin GPIO-Pin des Servos.
   */
  void attach(uint8_t pin);

  /**
   * @brief Trennt die Verbindung zum Servo (deaktiviert das PWM-Signal).
   */
  void detach();

  /**
   * @brief Trennt die Verbindung zum Servo, wenn dieser stillsteht und sich in
   * der Nullposition befindet.
   *
   * Schont den Motor und verhindert Zittern oder Summen im Ruhezustand.
   */
  void detachIfIdle();

private:
  /**
   * @brief Ermittelt den Stellplatz-Index (0 bis NUM_SPOTS-1) anhand eines
   * Servowinkels.
   * @param pos Der Servowinkel (in Grad).
   * @return Stellplatz-Index oder -1, wenn der Winkel ungültig ist.
   */
  int8_t getSpotIndex(uint8_t pos) const;

  MoToServo
      _servo1; ///< Der MobaTools-Servo für die physische Arm-Positionierung
  int16_t _currentTarget = -1; ///< Aktuell angefahrene Servoposition (-1 = keine)
};
