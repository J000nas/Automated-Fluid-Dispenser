#pragma once

#include <Arduino.h>
#include <stdint.h>

// ==========================================
// config.h - Zentrale Pin- und Wertverwaltung
// ==========================================

// --- PINS ---
constexpr uint8_t PIN_LEDS = 2;              // WS2811 Datenleitung
constexpr uint8_t PIN_SERVO = 3;             // Servo Motor
constexpr uint8_t PIN_START_TASTER = 4;      // Start Taster
constexpr uint8_t PIN_START_TASTER_LAMP = 5; // Lampe im Start Taster
constexpr uint8_t PIN_PUMP_TASTER = 6;       // Pumpen Taster
constexpr uint8_t PIN_PUMP_TASTER_LAMP = 7;  // Lampe im Pumpen Taster
constexpr uint8_t PIN_LEDS_TOWER = 9;        // Lampen Turm
constexpr uint8_t PIN_LEDS_RKL = 10;         // Rot-Kugel-Licht
constexpr uint8_t PIN_LEDS_FRONT_BACK = 11;  // Lampen vorne, hinten
constexpr uint8_t PIN_PUMP_ENABLE =
    12; // TMC2209 Treiber Enable (Active-LOW, ehemals Relais)
constexpr uint8_t PIN_PUMP_STEP = A7; // TMC2209 Step (A6 / D20 auf Nano Every)
constexpr uint8_t PIN_PUMP_DIR = A6;  // TMC2209 Dir (A7 / D21 auf Nano Every)
constexpr bool PUMP_INVERT_DIR = true; // Pumprichtung umkehren (true = umgekehrte Drehrichtung)
constexpr uint8_t PIN_LAMP_TASTER = 14;      // Lampen Taster
constexpr uint8_t PIN_LAMP_TASTER_LAMP = 15; // Lampe im Lampen Taster

constexpr uint8_t NUM_SPOTS = 5; // Anzahl der Stellplätze/Gläser
constexpr uint8_t LEDS_PER_SPOT =
    4; // Anzahl der LEDs pro Stellplatz (früher 2)
constexpr uint8_t TOTAL_LEDS =
    NUM_SPOTS * LEDS_PER_SPOT; // Gesamtzahl der LEDs auf dem Streifen

// --- EINSTELLUNGEN ---
constexpr uint8_t SERVO_PARK_POS = 130; // Ruheposition des Servos (ca. 6 Uhr)
constexpr uint8_t SERVO_POS[NUM_SPOTS] = {0, 5, 40, 75,
                                          100}; // Winkel für die Gläser 1 bis 5
constexpr unsigned long BLINK_INTERVAL =
    1280; // Blinkgeschwindigkeit synchron zum LED-Pulsieren (1280 ms Halbwelle
          // / 2560 ms Periode)
constexpr unsigned long WAIT_TIME = 11000; // Gesamte Wartezeit pro Glas (ms)
constexpr unsigned long WAIT_TIME_PUMP = 10000; // Pumpzeit pro Glas (ms)
constexpr uint16_t SERVO_SPEED_TIME =
    900; // Fahrzeit des Servos für 180 Grad in ms