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

bool toggleInInterval(unsigned long interval) {
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
