#include "Taster.h"

bool TasterStart() {
  // Statische Variablen behalten ihren Wert über Funktionsaufrufe hinweg
  static bool buttonStartOn =
      false; // Speichert, ob der Taster aktuell gedrückt gehalten wird
  static unsigned long buttonStartMillis = 0; // Zeitstempel für die Entprellung
  static bool toggleState =
      false; // Speichert den aktuellen Systemzustand (an/aus)

  // Taster auslesen (aktiv LOW durch internen Pullup-Widerstand)
  bool buttonStartRead = digitalRead(PIN_START_TASTER);

  // Wenn der Taster nicht gedrückt ist (HIGH), setzen wir den Entprell-Timer
  // kontinuierlich zurück
  if (buttonStartRead) {
    buttonStartMillis = millis();
  }

  // Wenn der Taster gedrückt wird (LOW), prüfen wir die Entprellzeit
  // (mindestens 20 ms gedrückt gehalten)
  if (!buttonStartRead && !buttonStartOn && millis() - buttonStartMillis > 20) {
    buttonStartOn = true;
  }

  // Flankenerkennung beim Loslassen des Tasters (Wechsel von LOW auf HIGH)
  if (buttonStartRead && buttonStartOn) {
    buttonStartOn = false;
    toggleState = !toggleState; // Umschalten des System-Betriebszustands
    return toggleState;
  }

  return toggleState; // Standardmäßig den aktuellen Zustand zurückgeben
}

bool toggleInInterval(uint16_t interval) {
  static bool state = false;
  static unsigned long lastToggleTime = 0;

  unsigned long currentTime = millis();
  // Prüfen, ob das gewünschte Intervall seit dem letzten Zustandswechsel
  // vergangen ist
  if (currentTime - lastToggleTime >= interval) {
    lastToggleTime = currentTime;
    state = !state; // Zustand wechseln (true -> false / false -> true)
  }
  return state;
}

void TasterLamp() {
  static bool buttonOn = false;
  static unsigned long buttonMillis = 0;
  static uint8_t lampMode = 0; // 0 = Aus, 1 = nur RKL, 2 = RKL + Front/Back, 3 = nur Front/Back
  static bool rklEnabled = false;

  bool buttonRead = digitalRead(PIN_LAMP_TASTER);

  if (buttonRead) {
    buttonMillis = millis();
  }

  // Taste gedrückt (mindestens 20 ms stabil)
  if (!buttonRead && !buttonOn && (millis() - buttonMillis > 20)) {
    buttonOn = true;
  }

  // Flankenerkennung beim Loslassen des Tasters
  if (buttonRead && buttonOn) {
    buttonOn = false;
    lampMode = (lampMode + 1) % 4;

    switch (lampMode) {
    case 0: // Alles AUS (0 %)
      rklEnabled = false;
      digitalWrite(PIN_LEDS_FRONT_BACK, LOW);
      setLampTasterBrightness(0);
      Serial.println(F("[LAMP] Zustand 0: Alles AUS (0%)"));
      break;

    case 1: // Nur RKL AN (Schwach gedimmt / Glimmen)
      rklEnabled = true;
      digitalWrite(PIN_LEDS_FRONT_BACK, LOW);
      setLampTasterBrightness(15); // ~6% Duty
      Serial.println(F("[LAMP] Zustand 1: Nur RKL AN (Schwach / 15)"));
      break;

    case 2: // RKL + FRONT_BACK AN (100 % volle Helligkeit)
      rklEnabled = true;
      digitalWrite(PIN_LEDS_FRONT_BACK, HIGH);
      setLampTasterBrightness(255); // 100%
      Serial.println(F("[LAMP] Zustand 2: RKL + Front/Back AN (Voll / 255)"));
      break;

    case 3: // Nur FRONT_BACK AN (Dunklere Mittelstufe)
      rklEnabled = false;
      digitalWrite(PIN_LEDS_FRONT_BACK, HIGH);
      setLampTasterBrightness(40); // ~16% Duty
      Serial.println(F("[LAMP] Zustand 3: Nur Front/Back AN (Mittel / 40)"));
      break;
    }
  }

  // --- Realistische Doppelblitz-Rundumleuchte (Doppel-Strobe mit Ausklingen) ---
  if (rklEnabled) {
    uint16_t cyclePos = millis() % 800; // 800 ms Gesamtperiode (ca. 1.25 Hz)
    if (cyclePos < 60) {
      // 1. Blitz (100 %)
      setRklBrightness(255);
    } else if (cyclePos < 120) {
      // Pause zwischen den Blitzen
      setRklBrightness(0);
    } else if (cyclePos < 180) {
      // 2. Blitz (100 %)
      setRklBrightness(255);
    } else if (cyclePos < 240) {
      // Sanftes Ausglimmen / Nachleuchten (255 -> 0)
      uint8_t fade = map(cyclePos, 180, 240, 255, 0);
      setRklBrightness(fade);
    } else {
      // Dunkelpause bis zum nächsten Doppelblitz
      setRklBrightness(0);
    }
  } else {
    setRklBrightness(0);
  }
}
