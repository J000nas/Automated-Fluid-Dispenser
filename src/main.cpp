#include "AnalogLimit.h"
#include "LedControl.h"
#include "Move.h"
#include "Queue.h"
#include "Taster.h"
#include "config.h"
#include <Arduino.h>

// --- Globale Variablen & Instanzen ---
bool first = true; ///< Flagge für die einmalige Start-Signalisierung (alle LEDs
                   ///< grün) bei Systemaktivierung
bool help_pump = false; ///< Hilfsflagge, um beim Stoppen die Pumpe abzuschalten
                        ///< und den Servo auf Null zu fahren

Queue queue(5); ///< Warteschlange für die Stellplätze (Größe = 5 Stellplätze)
LedControl
    led(10); ///< Steuerung für die 10 LEDs (jeweils 2 LEDs pro Stellplatz)
Move move;   ///< Ablauf- und Bewegungssteuerung (Servo-Motor & Pumpe)
AnalogLimit
    analogLimit; ///< Kalibrierung und Grenzwertverwaltung der analogen Sensoren

void setup() {
  Serial.begin(9600); // Serielle Kommunikation starten

  // Starttaster-Lampe und Taster initialisieren
  digitalWrite(PIN_START_TASTER_LAMP, HIGH);
  pinMode(PIN_START_TASTER, INPUT_PULLUP);
  pinMode(PIN_START_TASTER_LAMP, OUTPUT);

  // Hardware-Komponenten und LEDs initialisieren
  led.ledStart();          // Start-Animation der LEDs abspielen
  move.begin();            // Motorsteuerungs-Pins konfigurieren
  analogLimit.calibrate(); // Bechersensoren im unbeladenen Zustand kalibrieren
}

void loop() {
  static bool wasRunning =
      false; // Speichert den Betriebszustand des vorherigen Schleifendurchlaufs
  bool running =
      TasterStart(); // Abfrage des Starttasters (liefert getoggeltes Signal)

  // =========================================================================
  // SYSTEM AKTIV: Abfüllautomatik läuft
  // =========================================================================
  if (running) {
    // Zustandswechsel: System wurde soeben gestartet
    if (!wasRunning) {
      move.attach(PIN_SERVO); // Servo ankoppeln (PWM aktivieren)
      wasRunning = true;
      Serial.println(F("[MAIN] System gestartet - Servo angekoppelt."));
    }

    // Einmaliges optisches Feedback bei Systemstart: alle LEDs leuchten kurz
    // grün
    if (first) {
      first = false;
      led.setAll(CRGB::Green);
      delay(1000);
      led.clear();
    }

    // Die Lampe des Starttasters leuchtet durchgehend bei aktivem System
    digitalWrite(PIN_START_TASTER_LAMP,
                 LOW); // LED-Pin zieht nach GND (aktiv LOW)

    // Sensorabfrage und Ausführung der Abfüll-Zustandsmaschine
    move.status(queue, led,
                analogLimit); // Bechersensoren auswerten und in Queue eintragen
    move.run(queue, led);     // Abfüll-Ablaufsteuerung takten
    queue.printQueue();       // Warteschlange im Intervall seriell ausgeben

    help_pump = true;
    delay(50); // Kleiner Delay zur Entlastung des Controllers

  }
  // =========================================================================
  // SYSTEM INAKTIV (Standby): Wartet auf Start, manuelles Spülen möglich
  // =========================================================================
  else {
    // Zustandswechsel: System wurde soeben gestoppt
    if (wasRunning) {
      wasRunning = false;
      Serial.println(
          F("[MAIN] System gestoppt - Fahre zurueck auf Position 0..."));
    }
    first =
        true; // Start-Feedback-Flag für den nächsten Systemstart zurücksetzen

    // Wenn das System gestoppt wurde, Pumpe ausschalten und Servo parken
    if (help_pump) {
      digitalWrite(PIN_PUMP_RELAY, LOW);
      help_pump = false;
      move.toZero();
    }

    // Servo im Leerlauf abkoppeln, um Hitzeentwicklung und Zittern zu vermeiden
    move.detachIfIdle();

    // Blinkende Signalisierung der Starttaster-Lampe im Standby-Modus
    if (toggleInInterval(BLINK_INTERVAL)) {
      digitalWrite(PIN_START_TASTER_LAMP, LOW);
    } else {
      digitalWrite(PIN_START_TASTER_LAMP, HIGH);
    }

    // Im Standby-Modus manuelles Vorpumpen/Spülen via Taster erlauben
    move.pump();

    // Status-Blinken aller Stellplatz-LEDs (Gelb) im Standby-Modus
    led.blink(BLINK_INTERVAL, CRGB::Yellow);
  }
}