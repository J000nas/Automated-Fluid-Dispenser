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

// --- Flackerfreie PWM für Start-/Lampentaster und RKL (über Timer TCB0) ---
static volatile uint8_t g_startLampDuty = 0;
static volatile uint8_t g_lampTasterDuty = 0;
static volatile uint8_t g_rklDuty = 0;

ISR(TCB0_INT_vect) {
  TCB0.INTFLAGS = TCB_CAPT_bm; // Interrupt-Flag quittieren
  static uint8_t pwmCount = 0;
  pwmCount++;

  // 1) Start-Taster Lampe (Pin 5 / PB2)
  if (pwmCount < g_startLampDuty) {
    VPORTB.OUT |= (1 << 2); // PIN_START_TASTER_LAMP HIGH
  } else {
    VPORTB.OUT &= ~(1 << 2); // PIN_START_TASTER_LAMP LOW
  }

  // 2) Lampen-Taster Lampe (Pin 15 / A1 / PD2)
  if (pwmCount < g_lampTasterDuty) {
    VPORTD.OUT |= (1 << 2); // PIN_LAMP_TASTER_LAMP HIGH
  } else {
    VPORTD.OUT &= ~(1 << 2); // PIN_LAMP_TASTER_LAMP LOW
  }

  // 3) RKL Rundumleuchte (Pin 10 / PB1)
  if (pwmCount < g_rklDuty) {
    VPORTB.OUT |= (1 << 1); // PIN_LEDS_RKL HIGH
  } else {
    VPORTB.OUT &= ~(1 << 1); // PIN_LEDS_RKL LOW
  }
}

void initStartLampPwm() {
  pinMode(PIN_START_TASTER_LAMP, OUTPUT);
  digitalWrite(PIN_START_TASTER_LAMP, LOW);

  pinMode(PIN_LAMP_TASTER_LAMP, OUTPUT);
  digitalWrite(PIN_LAMP_TASTER_LAMP, LOW);

  pinMode(PIN_LEDS_RKL, OUTPUT);
  digitalWrite(PIN_LEDS_RKL, LOW);

  // TCB0 im Periodic-Interrupt-Modus auf 62.5 kHz (16 MHz / 256)
  // 62.5 kHz / 256 Schritte = 244 Hz Grundfrequenz bei Duty 1 -> absolut flackerfrei
  TCB0.CCMP = 256;
  TCB0.CTRLB = TCB_CNTMODE_INT_gc;
  TCB0.INTCTRL = TCB_CAPT_bm;
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
}

static inline void setStartLampBrightness(uint8_t brightness) {
  g_startLampDuty = brightness;
}

void setLampTasterBrightness(uint8_t brightness) {
  g_lampTasterDuty = brightness;
}

void setRklBrightness(uint8_t brightness) {
  g_rklDuty = brightness;
}

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

  // Starttaster und Lampen-PWM initialisieren
  pinMode(PIN_START_TASTER, INPUT_PULLUP);
  initStartLampPwm();

  pinMode(PIN_PUMP_TASTER, INPUT_PULLUP);
  pinMode(PIN_PUMP_TASTER_LAMP, OUTPUT);
  digitalWrite(PIN_PUMP_TASTER_LAMP, LOW);

  pinMode(PIN_LAMP_TASTER, INPUT_PULLUP);

  pinMode(PIN_LEDS_FRONT_BACK, OUTPUT);
  digitalWrite(PIN_LEDS_FRONT_BACK, LOW);

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

  // Beleuchtungs-Taster abfragen (RKL / FRONT_BACK)
  TasterLamp();

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
      wasRunning = true;
      Serial.println(F("[MAIN] System gestartet."));
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
    setStartLampBrightness(255);

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
      led.clear(); // Setzt Standby-Timer und Stellplatz-LEDs sauber zurück
      Serial.println(
          F("[MAIN] System gestoppt - Fahre zurueck auf Position 0..."));
    }
    first =
        true; // Start-Feedback-Flag für den nächsten Systemstart zurücksetzen

    // Wenn das System gestoppt wurde, Pumpe ausschalten und Servo parken
    if (help_pump) {
      digitalWrite(PIN_PUMP_RELAY, LOW);
      digitalWrite(PIN_PUMP_TASTER_LAMP, LOW);
      digitalWrite(PIN_LEDS_TOWER, LOW);
      help_pump = false;
      move.toZero();
    }

    // Servo im Leerlauf abkoppeln, um Hitzeentwicklung und Zittern zu vermeiden
    move.detachIfIdle();

    // Im Standby-Modus manuelles Vorpumpen/Spülen via Taster erlauben
    move.pump();

    // Weiches blaues Pulsieren (Atem-Effekt) aller Stellplatz-LEDs im
    // Standby-Modus
    led.showStandbyPulse(CRGB::Blue);

    // Starttaster-Lampe stufenlos & synchron zum Pulsieren der LEDs dimmen
    setStartLampBrightness(led.getStandbyPulse());
  }
}