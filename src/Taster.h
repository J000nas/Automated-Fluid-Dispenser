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
 * @brief Erzeugt ein regelmäßiges Blinksignal (Toggle) ohne den Programmfluss
 * zu blockieren.
 *
 * @param interval Blink-Intervall in Millisekunden.
 * @return true in der einen Hälfte des Intervalls, false in der anderen Hälfte.
 */
bool toggleInInterval(unsigned long interval);
