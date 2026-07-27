#pragma once

#include <stdint.h>

// ==========================================
// config.h - Zentrale Pin- und Wertverwaltung
// ==========================================

// --- PINS ---
constexpr uint8_t PIN_START_TASTER      = 7;  // Start Taster
constexpr uint8_t PIN_START_TASTER_LAMP = 6;  // Lampe im Start Taster
constexpr uint8_t PIN_PUMP_RELAY        = 12; // Relais für die Pumpe
constexpr uint8_t PIN_PUMP_TASTER       = 5;  // Manueller Pumpen-Taster
constexpr uint8_t PIN_SERVO             = 9;  // Servo Motor
constexpr uint8_t PIN_LEDS              = 10; // WS2811 Datenleitung

constexpr uint8_t NUM_SPOTS = 5;                      // Anzahl der Stellplätze/Gläser
constexpr uint8_t LEDS_PER_SPOT = 4;                  // Anzahl der LEDs pro Stellplatz (früher 2)
constexpr uint8_t TOTAL_LEDS = NUM_SPOTS * LEDS_PER_SPOT; // Gesamtzahl der LEDs auf dem Streifen

// --- EINSTELLUNGEN ---
constexpr uint8_t SERVO_POS[NUM_SPOTS] = {155, 127, 96, 68, 42}; // Winkel für die Gläser
constexpr unsigned long BLINK_INTERVAL = 700;                 // Blinkgeschwindigkeit
constexpr unsigned long WAIT_TIME = 11000;                    // Gesamte Wartezeit pro Glas (ms)
constexpr unsigned long WAIT_TIME_PUMP = 10000;               // Pumpzeit pro Glas (ms)