#include "CupSensorManager.h"
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
    led(TOTAL_LEDS); ///< Steuerung für die LEDs (konfiguriert via config.h)
Move move;           ///< Ablauf- und Bewegungssteuerung (Servo-Motor & Pumpe)
CupSensorManager sensorManager; ///< Kapazitive Glaserkennung über den MPR121

// --- Debug-Ausgabe für Glaserkennung & Queue ---
void printDebugStatus(CupSensorManager &sensor, Queue &q) {
  static unsigned long lastDebugPrint = 0;
  const unsigned long DEBUG_INTERVAL = 500; // Alle 500ms ausgeben
  unsigned long now = millis();
  if (now - lastDebugPrint < DEBUG_INTERVAL)
    return;
  lastDebugPrint = now;
  Serial.println(F("=== DEBUG: Glaserkennung ==="));
  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    Serial.print(F("  Platz "));
    Serial.print(i);
    Serial.print(F(": Raw="));
    Serial.print(sensor.getRawDiff(i));
    Serial.print(F("  Komp="));
    Serial.print(sensor.getCompensatedDiff(i));
    Serial.print(F("  Glas="));
    Serial.println(sensor.isCupDetected(i) ? F("[XXX]") : F("[---]"));
  }
  Serial.print(F("=== Queue: "));
  Serial.print(q.queueSize());
  Serial.print(F("/"));
  Serial.print(q.size());
  Serial.println(F(" ==="));
}

void setup() {
  Serial.begin(115200); // Serielle Kommunikation mit hoher Baudrate starten
                        // (verhindert Loop-Blockaden)

  // Starttaster-Lampe und Taster initialisieren
  pinMode(PIN_START_TASTER, INPUT_PULLUP);
  pinMode(PIN_START_TASTER_LAMP, OUTPUT);
  digitalWrite(PIN_START_TASTER_LAMP, HIGH);

  // Hardware-Komponenten und LEDs initialisieren
  led.ledStart(); // Start-Animation der LEDs abspielen
  move.begin();   // Motorsteuerungs-Pins konfigurieren

  // Kapazitiven Sensor initialisieren
  if (!sensorManager.begin()) {
    Serial.println(F(
        "[MAIN] FATAL: Cupsensor (MPR121) konnte nicht initialisiert werden!"));
    // System anhalten und Fehler mit rot blinkenden LEDs signalisieren
    while (true) {
      led.setAll(CRGB::Red);
      delay(500);
      led.clear();
      delay(500);
    }
  }
}

void loop() {
  // Immer den Cupsensor aktualisieren (wichtig für Baseline-Drift-Tracking im
  // Standby)
  sensorManager.update();
  printDebugStatus(sensorManager, queue); // Debug-Ausgabe für Glaserkennung

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
    move.status(
        queue, led,
        sensorManager);   // Bechersensoren auswerten und in Queue eintragen
    move.run(queue, led); // Abfüll-Ablaufsteuerung takten
    led.update();         // LED-Animationen berechnen (Fading, Wellen)
    queue.printQueue();   // Warteschlange im Intervall seriell ausgeben
    help_pump = true;
    delay(5); // Kleiner Delay zur Entlastung (erhöht Loop-Frequenz für flüssige
              // LED-Animationen)

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

    // Weiches blaues Pulsieren (Atem-Effekt) aller Stellplatz-LEDs im
    // Standby-Modus
    led.showStandbyPulse(CRGB::Blue);
  }
}