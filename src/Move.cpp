#include "Move.h"

Move::Move() {
  // Richtet alle analogen Sensor-Pins als Eingang mit Pullup-Widerständen ein
  for (int i : PIN_SENSORS) {
    pinMode(i, INPUT_PULLUP);
  }
}

void Move::status(Queue &queue, LedControl &led, AnalogLimit &analogLimit) {
  // Statische Zustände speichern, um Zustandswechsel (Flanken) zu erkennen
  static bool lastSensorState[NUM_SPOTS] = {false};
  static bool sensorTriggered[NUM_SPOTS] = {false};
  static unsigned long sensorActiveSince[NUM_SPOTS] = {0};

  for (int i = 0; i < NUM_SPOTS; i++) {
    bool value;

    // Wenn der analoge Sensorwert kleiner oder gleich dem kalibrierten
    // Grenzwert ist, steht ein Glas auf dem Stellplatz (Wert zieht gegen GND).
    if (analogRead(PIN_SENSORS[i]) <= analogLimit.getValue(i)) {
      value = true;
    } else {
      value = false;
    }

    // Steigende Flanke: Glas wurde gerade hingestellt
    if (value) {
      if (!lastSensorState[i]) {
        // Startzeit der Erkennung merken (für Entprellung)
        sensorActiveSince[i] = millis();
      }

      // Entprellung: Glas muss mindestens 200 ms stabil erkannt werden
      if (!sensorTriggered[i] && (millis() - sensorActiveSince[i] >= 200)) {
        queue.addToQueue(SERVO_POS[i]); // Servoposition in Queue eintragen
        led.setColor(i + 1,
                     CRGB::Red); // LED rot leuchten lassen (Becher erkannt)
        sensorTriggered[i] = true;
      }
    }

    // Fallende Flanke: Glas wurde angehoben/entfernt
    if (!value && lastSensorState[i]) {
      queue.removeFromQueue(SERVO_POS[i]); // Servoposition aus Queue entfernen
      led.setColor(i + 1, CRGB::Black);    // LED ausschalten
      sensorTriggered[i] = false;
    }

    // Aktuellen Status für den nächsten Durchlauf speichern
    lastSensorState[i] = value;
  }
}

void Move::moveToNext(const Queue &queue) {
  int targetPos = queue.getNextPosition();
  if (targetPos >= 0) {
    // Nur anfahren, wenn sich der Servo nicht bereits bewegt
    if (!_servo1.moving()) {
      _servo1.write(targetPos);
    }
  }
}

void Move::pump() {
  static bool lastButtonState = HIGH;
  static bool pumpOn = false;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  // Manuellen Taster zum Vorpumpen auslesen (aktiv LOW)
  bool currentState = digitalRead(PIN_PUMP_TASTER);

  if (currentState != lastButtonState) {
    lastDebounceTime = millis(); // Timer bei Flankenwechsel zurücksetzen
  }

  // Wenn der Zustand stabil geblieben ist (Entprellzeit abgelaufen)
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Taster wurde frisch gedrückt (LOW)
    if (currentState == LOW && !pumpOn) {
      digitalWrite(PIN_PUMP_RELAY, HIGH); // Relais/Pumpe einschalten
      pumpOn = true;
      Serial.println("PUMPE AN (manuell)");
    }
    // Taster wurde losgelassen (HIGH)
    else if (currentState == HIGH && pumpOn) {
      digitalWrite(PIN_PUMP_RELAY, LOW); // Relais/Pumpe ausschalten
      pumpOn = false;
      Serial.println("PUMPE AUS (manuell)");
    }
  }

  lastButtonState = currentState;
}

void Move::run(Queue &queue, LedControl &led) {
  // Zustände der State-Machine zur sequentiellen Ablaufsteuerung der Abfüllung
  static enum State { IDLE, MOVING, WAITING_PUMP, WAITING_NEXT } state = IDLE;
  static unsigned long lastMoveTime = 0;
  static long lastTarget = -1; // Letzte tatsächlich angefahrene Position

  switch (state) {
  case IDLE:
    // Prüfen, ob Aufträge in der Warteschlange vorliegen
    if (queue.queueSize() > 0) {
      long nextPos = queue.getNextPosition();
      if (nextPos >= 0) {
        _servo1.write(nextPos); // Servo fährt zur ersten Position
        Serial.print("[RUN] Neue Position angefordert: ");
        Serial.println(nextPos);
        state = MOVING;
      } else {
        Serial.println("[RUN] Fehler: Ungueltige Position (-1) erkannt.");
      }
    } else {
      // Wenn die Warteschlange leer ist, fährt der Servo in die Parkposition (0
      // Grad) zurück
      if (_servo1.read() != 0) {
        _servo1.write(0);
        Serial.println("[RUN] Warteschlange leer, Motor faehrt zurueck auf 0.");
      }
      digitalWrite(PIN_PUMP_RELAY,
                   LOW); // Sicherstellen, dass die Pumpe aus ist
    }
    break;

  case MOVING:
    // Warten, bis der Servo seine Zielposition erreicht hat (moving == 0)
    if (_servo1.moving() == 0) {
      Serial.println("[RUN] Zielposition erreicht, Pumpe wird aktiviert.");

      Serial.print("[DEBUG] Erreichte Position: ");
      Serial.println(_servo1.read());

      long nextPos = queue.getNextPosition();
      if (nextPos >= 0) {
        int spotIdx = getSpotIndex(nextPos);
        if (spotIdx >= 0) {
          led.setColor(spotIdx + 1,
                       CRGB::Yellow); // LED gelb leuchten lassen (wird befüllt)
        } else {
          Serial.println(F("[RUN] Warnung: Position unbekannt."));
        }

        digitalWrite(PIN_PUMP_RELAY, HIGH); // Pumpe starten
        lastMoveTime = millis(); // Startzeit des Abfüllvorgangs speichern
        state = WAITING_PUMP;
      } else {
        Serial.println("[RUN] Fehler: Keine gueltige naechste Position.");
        state = IDLE;
      }
    }
    break;

  case WAITING_PUMP:
    // Warten, bis die vordefinierte Pumpzeit (WAIT_TIME_PUMP) abgelaufen ist
    if (millis() - lastMoveTime >= WAIT_TIME_PUMP) {
      digitalWrite(PIN_PUMP_RELAY, LOW); // Pumpe stoppen
      Serial.println("[RUN] Pumpzeit abgelaufen, Pumpe deaktiviert.");

      long nextPos = queue.getNextPosition();
      if (nextPos >= 0) {
        int spotIdx = getSpotIndex(nextPos);
        if (spotIdx >= 0) {
          led.setColor(
              spotIdx + 1,
              CRGB::Green); // LED grün leuchten lassen (fertig befüllt)
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
    // Gesamte Wartezeit abwarten, damit Flüssigkeit abtropfen kann (WAIT_TIME
    // abzüglich der Pumpzeit)
    if (millis() - lastMoveTime >= WAIT_TIME) {
      Serial.println("[RUN] Gesamte Wartezeit abgelaufen, naechste Position.");

      Serial.print("[DEBUG] Vor Queue.popFront() – Position: ");
      Serial.println(_servo1.read());

      // Den soeben befüllten Becher aus der Warteschlange entfernen
      queue.popFront();
      state = IDLE; // Bereit für den nächsten Befüllvorgang
    }
    break;
  }
}

void Move::toZero() {
  _servo1.write(0); // Parkposition (0 Grad) anfahren
}

void Move::detach() {
  _servo1.detach(); // Deaktiviert das PWM-Signal des Servos (schont die
                    // Zahnräder und spart Strom)
}

void Move::detachIfIdle() {
  // Koppelt den Servo ab, wenn er sich in Parkposition (0) befindet und die
  // Bewegung abgeschlossen ist
  if (_servo1.attached() && _servo1.read() == 0 && _servo1.moving() == 0) {
    _servo1.detach();
    Serial.println(F("[MOVE] Servo erfolgreich im Leerlauf abgekoppelt."));
  }
}

void Move::attach(int pin) {
  _servo1.attach(pin); // Servo wieder an Pin koppeln
  _servo1.setSpeedTime(
      1100); // Fahrzeit für 180 Grad auf 1.1s festlegen (sanfte Fahrt)
}

void Move::begin() {
  pinMode(PIN_PUMP_RELAY, OUTPUT);
  pinMode(PIN_PUMP_TASTER, INPUT_PULLUP);
  digitalWrite(PIN_PUMP_RELAY,
               LOW); // Sicherstellen, dass das Relais zu Beginn aus ist
}

int Move::getSpotIndex(int pos) const {
  // Sucht den Stellplatz-Index (0 bis NUM_SPOTS-1) für eine gegebene
  // Servoposition
  for (int i = 0; i < NUM_SPOTS; i++) {
    if (SERVO_POS[i] == pos) {
      return i;
    }
  }
  return -1;
}