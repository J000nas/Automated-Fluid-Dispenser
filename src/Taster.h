#pragma once

#include "config.h"
#include <Arduino.h>

/**
 * @file Taster.h
 * @brief Enthält Hilfsfunktionen zur Abfrage des Starttasters und für
 * zeitgesteuerte Blinksignale.
 */

/**
 * @brief Liest den Status des Start-Tasters ein und toggelt den Systemzustand.
 *
 * Beinhaltet eine Software-Entprellung (20 ms). Jedes vollständige Drücken und
 * Loslassen des Tasters schaltet den Rückgabewert (System Aktiv / Inaktiv) um.
 *
 * @return true, wenn das System aktiv sein soll, false, wenn es gestoppt ist.
 */
bool TasterStart();

/**
 * @brief Fragt den Lampen-Taster ab und schaltet die Beleuchtungsmodi durch:
 * 0 = Alles AUS
 * 1 = Nur RKL AN
 * 2 = RKL + FRONT_BACK AN
 * 3 = Nur FRONT_BACK AN
 * 4 = Wieder Alles AUS (Zyklus)
 */
void TasterLamp();

/**
 * @brief Setzt die Helligkeit der Lampe im Lampen-Taster (0 bis 255).
 * @param brightness Helligkeitswert (0 = Aus, 255 = 100 %).
 */
void setLampTasterBrightness(uint8_t brightness);

/**
 * @brief Setzt die Helligkeit / PWM der RKL-Rundumleuchte (0 bis 255).
 * @param brightness Helligkeitswert (0 = Aus, 255 = 100 %).
 */
void setRklBrightness(uint8_t brightness);

/**
 * @brief Erzeugt ein regelmäßiges Blinksignal (Toggle) ohne den Programmfluss
 * zu blockieren.
 *
 * @param interval Blink-Intervall in Millisekunden.
 * @return true in der einen Hälfte des Intervalls, false in der anderen Hälfte.
 */
bool toggleInInterval(uint16_t interval);
