//
// Created by Jonas Will on 09.05.25.
//

#include "Move.h"


/* --Konstruktor */
Move::Move(){
    for (int i : PIN_SENSORS) {
        pinMode(i, INPUT_PULLUP);
    }
}

/* --Methode zum Abrufen vom Status der Sensoren */
void Move::status(Queue& queue, LedControl& led, AnalogLimit& analogLimit) {
    static bool lastSensorState[NUM_SPOTS] = {false};
    static bool sensorTriggered[NUM_SPOTS] = {false};
    static unsigned long sensorActiveSince[NUM_SPOTS] = {0};

    for (int i = 0; i < NUM_SPOTS; i++) {
        bool value;

        if (analogRead(PIN_SENSORS[i])<= analogLimit.getValue(i)) {
            value = true;
        }else {
            value = false;
        }


        if (value) {
            if (!lastSensorState[i]) {
                // Sensor wurde gerade aktiv – Startzeit merken
                sensorActiveSince[i] = millis();
            }

            // Wenn der Sensor mindestens 100ms aktiv war
            if (!sensorTriggered[i] && (millis() - sensorActiveSince[i] >= 200)) {
                queue.addToQueue(SERVO_POS[i]);
                led.setColor(i + 1, CRGB::Red);
                sensorTriggered[i] = true;
            }
        }

        // Wenn Sensor inaktiv wird (Falling Edge)
        if (!value && lastSensorState[i]) {
            queue.removeFromQueue(SERVO_POS[i]);
            led.setColor(i + 1, CRGB::Black);
            sensorTriggered[i] = false;
        }

        // Status speichern für nächsten Durchlauf
        lastSensorState[i] = value;
    }
}


/* --Methode um zur Position zu fahren */
void Move::moveToNext(const Queue &queue) {
    int targetPos = queue.getNextPosition();
    if (targetPos >= 0) {
        if (!_servo1.moving()) {
            _servo1.write(targetPos);
        }
    }
}

/* --Funktion um die Pumpe zum Vorpumpen anzusteuern */
void Move::pump() {
    static bool lastButtonState = HIGH;
    static bool pumpOn = false;
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;

    bool currentState = digitalRead(PIN_PUMP_TASTER);
    //Serial.println(currentState);  // Zum Debuggen

    if (currentState != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        // Wenn sich der Tasterzustand stabilisiert hat
        if (currentState == LOW && !pumpOn) {
            // Taster wurde gedrückt
            digitalWrite(PIN_PUMP_RELAY, HIGH);
            pumpOn = true;
            Serial.println("PUMPE AN");
        } else if (currentState == HIGH && pumpOn) {
            // Taster wurde losgelassen
            digitalWrite(PIN_PUMP_RELAY, LOW);
            pumpOn = false;
            Serial.println("PUMPE AUS");
        }
    }

    lastButtonState = currentState;
}

/* --Methode zum Fahren des Motors */
void Move::run(Queue& queue, LedControl& led) {
    static enum State { IDLE, MOVING, WAITING_PUMP, WAITING_NEXT } state = IDLE;
    static unsigned long lastMoveTime = 0;

    static long lastTarget = -1;  // [GEÄNDERT] Letzte tatsächlich angefahrene Position

    switch (state) {
        case IDLE:
            if (queue.queueSize() > 0) {
                long nextPos = queue.getNextPosition();
                if (nextPos >= 0) {
                    _servo1.write(nextPos);
                    Serial.print("[RUN] Neue Position angefordert: ");
                    Serial.println(nextPos);
                    state = MOVING;
                } else {
                    Serial.println("[RUN] Fehler: Ungueltige Position (-1) erkannt.");
                }
            } else {
                if (_servo1.read() != 0) {
                    _servo1.write(0);
                    Serial.println("[RUN] Warteschlange leer, Motor faehrt zurueck auf 0.");
                }
                digitalWrite(PIN_PUMP_RELAY, LOW);
            }
            break;

        case MOVING:
            if (_servo1.moving() == 0) {
                Serial.println("[RUN] Zielposition erreicht, Pumpe wird aktiviert.");

                // [DEBUG] aktuelle Position ausgeben
                Serial.print("[DEBUG] Erreichte Position: ");
                Serial.println(_servo1.read());

                long nextPos = queue.getNextPosition();
                if (nextPos >= 0) {
                    int spotIdx = getSpotIndex(nextPos);
                    if (spotIdx >= 0) {
                        led.setColor(spotIdx + 1, CRGB::Yellow);
                    } else {
                        Serial.println(F("[RUN] Warnung: Position unbekannt."));
                    }
                    digitalWrite(PIN_PUMP_RELAY, HIGH);
                    lastMoveTime = millis();
                    state = WAITING_PUMP;
                } else {
                    Serial.println("[RUN] Fehler: Keine gueltige naechste Position."); // [GEÄNDERT]
                    state = IDLE;
                }
            }
            break;

        case WAITING_PUMP:
            if (millis() - lastMoveTime >= WAIT_TIME_PUMP) {
                digitalWrite(PIN_PUMP_RELAY, LOW);
                Serial.println("[RUN] Pumpzeit abgelaufen, Pumpe deaktiviert.");
                long nextPos = queue.getNextPosition();
                if (nextPos >= 0) {
                    int spotIdx = getSpotIndex(nextPos);
                    if (spotIdx >= 0) {
                        led.setColor(spotIdx + 1, CRGB::Green);
                    } else {
                        Serial.println(F("[RUN] Warnung: Position unbekannt."));
                    }
                    state = WAITING_NEXT;
                } else {
                    Serial.println("[RUN] Fehler: Position ungueltig nach Pumpzeit.");
                    state = IDLE;
                }
            }
            break;

        case WAITING_NEXT:
            if (millis() - lastMoveTime >= WAIT_TIME) {
                Serial.println("[RUN] Gesamte Wartezeit abgelaufen, naechste Position.");

                // [DEBUG] aktuelle Position vor Queue-Wechsel
                Serial.print("[DEBUG] Vor Queue.popFront() – Position: ");
                Serial.println(_servo1.read());


                queue.popFront();
                state = IDLE;
            }
            break;
    }
}

/* --Methode um Servo auf Position 0 zu fahren */
void Move::toZero() {
    _servo1.write(0);
}

void Move::detach() {
    _servo1.detach();
}

void Move::detachIfIdle() {
    if (_servo1.attached() && _servo1.read() == 0 && _servo1.moving() == 0) {
        _servo1.detach();
        Serial.println(F("[MOVE] Servo erfolgreich im Leerlauf abgekoppelt."));
    }
}

void Move::attach(int pin) {
    _servo1.attach(pin); //Pin 9
    _servo1.setSpeedTime(1100);
}

/* --Methode zum Initialisieren des Motors */
void Move::begin() {
    pinMode(PIN_PUMP_RELAY, OUTPUT);
    pinMode(PIN_PUMP_TASTER, INPUT_PULLUP);
}

int Move::getSpotIndex(int pos) const {
    for (int i = 0; i < NUM_SPOTS; i++) {
        if (SERVO_POS[i] == pos) {
            return i;
        }
    }
    return -1;
}