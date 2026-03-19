//
// Created by Jonas Will on 09.05.25.
//

#include "Taster.h"

/* --Funktion zum Erkennen vom Status vom Start Taster */
bool TasterStart() {
    static bool buttonStartOn = false;
    static unsigned long buttonStartMillis = 0;
    static bool toggleState = false;

    bool buttonStartRead = digitalRead(PIN_START_TASTER);

    // Taster wird gedrückt
    if (buttonStartRead) {
        buttonStartMillis = millis();
    }

    // Entprellung: Taster wurde losgelassen, nach kurzem Druck
    if (!buttonStartRead && !buttonStartOn && millis() - buttonStartMillis > 20) {
        buttonStartOn = true;
    }

    // Taster wurde vollständig losgelassen
    if (buttonStartRead && buttonStartOn) {
        buttonStartOn = false;
        toggleState = !toggleState;  // Umschalten
        return toggleState;
    }

    return toggleState;  // Aktueller Zustand, keine Umschaltung
}

/* --Funktion für das Blinken */
bool toggleInInterval(unsigned long interval) {
    static bool state = false;
    static unsigned long lastToggleTime = 0;

    unsigned long currentTime = millis();
    if (currentTime - lastToggleTime >= interval) {
        lastToggleTime = currentTime;
        state = !state;  // Zustand wechseln
    }
    return state;
}
