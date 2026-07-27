#pragma once

// ==========================================
// config.h - Zentrale Pin- und Wertverwaltung
// ==========================================

// --- PINS ---
constexpr int PIN_START_TASTER      = 7;  // Start Taster
constexpr int PIN_START_TASTER_LAMP = 6;  // Lampe im Start Taster
constexpr int PIN_PUMP_RELAY        = 12; // Relais für die Pumpe
constexpr int PIN_PUMP_TASTER       = 5;  // Manueller Pumpen-Taster
constexpr int PIN_SERVO             = 9;  // Servo Motor
constexpr int PIN_LEDS              = 10; // WS2811 Datenleitung

constexpr int NUM_SPOTS = 5;                  // Anzahl der Stellplätze/Gläser

// Analog Sensor Pins (A0-A4 entsprechen den digitalen Pins 14-18)
constexpr int PIN_SENSORS[NUM_SPOTS] = {14, 15, 16, 17, 18};

// --- EINSTELLUNGEN ---
constexpr int SERVO_POS[NUM_SPOTS] = {155, 127, 96, 68, 42}; // Winkel für die Gläser
constexpr unsigned long BLINK_INTERVAL = 700;                 // Blinkgeschwindigkeit
constexpr unsigned long WAIT_TIME = 11000;                    // Gesamte Wartezeit pro Glas (ms)
constexpr unsigned long WAIT_TIME_PUMP = 10000;               // Pumpzeit pro Glas (ms)