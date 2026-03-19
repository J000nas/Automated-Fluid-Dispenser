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

// Analog Sensor Pins (A0-A4 entsprechen den digitalen Pins 14-18)
constexpr int PIN_SENSORS[5] = {14, 15, 16, 17, 18};

// --- EINSTELLUNGEN ---
constexpr int SERVO_POS[5] = {155, 127, 96, 68, 42}; // Winkel für die Gläser
constexpr unsigned long BLINK_INTERVAL = 700;        // Blinkgeschwindigkeit