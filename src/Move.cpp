#include "Move.h"

Move::Move() {
  // Der MobaTools-Servo wird initialisiert. Zusätzliche Pins für den
  // kapazitiven Sensor müssen nicht über pinMode konfiguriert werden,
  // da dies über den I2C-Bus (SDA/SCL) abgewickelt wird.
}

void Move::status(Queue &queue, LedControl &led,
                  CupSensorManager &sensorManager) {
  // Statische Zustände speichern, um Zustandswechsel (Flanken) zu erkennen
  static bool lastSensorState[NUM_SPOTS] = {false};
  static bool sensorTriggered[NUM_SPOTS] = {false};
  static unsigned long sensorActiveSince[NUM_SPOTS] = {0};

  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    // Wert direkt vom kapazitiven CupSensorManager abfragen
    bool value = sensorManager.isCupDetected(i);

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
      led.clearSpot(i + 1); // Fade-Out starten (sanftes Ausblenden)
      sensorTriggered[i] = false;
    }

    // Aktuellen Status für den nächsten Durchlauf speichern
    lastSensorState[i] = value;
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
      digitalWrite(PIN_PUMP_RELAY, HIGH);       // Relais/Pumpe einschalten
      digitalWrite(PIN_PUMP_TASTER_LAMP, HIGH);  // Lampe im Pumpentaster einschalten
      digitalWrite(PIN_LEDS_TOWER, HIGH);        // LED Tower einschalten
      pumpOn = true;
      Serial.println(F("PUMPE AN (manuell)"));
    }
    // Taster wurde losgelassen (HIGH)
    else if (currentState == HIGH && pumpOn) {
      digitalWrite(PIN_PUMP_RELAY, LOW);        // Relais/Pumpe ausschalten
      digitalWrite(PIN_PUMP_TASTER_LAMP, LOW);   // Lampe im Pumpentaster ausschalten
      digitalWrite(PIN_LEDS_TOWER, LOW);         // LED Tower ausschalten
      pumpOn = false;
      Serial.println(F("PUMPE AUS (manuell)"));
    }
  }

  lastButtonState = currentState;
}

void Move::run(Queue &queue, LedControl &led) {
  // Zustände der State-Machine zur sequentiellen Ablaufsteuerung der Abfüllung
  static enum State { IDLE, MOVING, WAITING_PUMP, WAITING_NEXT, MOVING_TO_ZERO } state = IDLE;
  static unsigned long stateStartTime = 0;

  switch (state) {
  case IDLE:
    // Prüfen, ob Aufträge in der Warteschlange vorliegen
    if (queue.queueSize() > 0) {
      int16_t nextPos = queue.getNextPosition();
      if (nextPos >= 0) {
        _currentTarget = nextPos; // Zielposition für den aktuellen Abfüllvorgang merken
        uint16_t diff = abs(nextPos - _currentArmAngle);
        _expectedMoveDuration = (uint16_t)(((unsigned long)diff * SERVO_SPEED_TIME) / 180) + 50;
        _currentArmAngle = nextPos;
        attach(PIN_SERVO);        // Servo ankoppeln mit konstanter Geschwindigkeit
        _servo1.write(nextPos);   // Servo fährt sanft zur Position
        stateStartTime = millis();
        state = MOVING;
        Serial.print(F("[RUN] Fahre zu Position "));
        Serial.print(nextPos);
        Serial.print(F(" Grad (Fahrzeit: "));
        Serial.print(_expectedMoveDuration);
        Serial.println(F(" ms)..."));
      } else {
        Serial.println(F("[RUN] Fehler: Ungueltige Position (-1) erkannt."));
      }
    } else {
      // Wenn die Warteschlange leer ist und der Arm noch nicht in Parkposition steht:
      if (_currentArmAngle != SERVO_PARK_POS) {
        uint16_t diff = abs(_currentArmAngle - SERVO_PARK_POS);
        _expectedMoveDuration = (uint16_t)(((unsigned long)diff * SERVO_SPEED_TIME) / 180) + 50;
        _currentArmAngle = SERVO_PARK_POS;
        attach(PIN_SERVO);
        _servo1.write(SERVO_PARK_POS);
        stateStartTime = millis();
        state = MOVING_TO_ZERO;
        Serial.print(F("[RUN] Warteschlange leer, fahre zurueck auf Parkposition "));
        Serial.print(SERVO_PARK_POS);
        Serial.print(F(" Grad (Dauer: "));
        Serial.print(_expectedMoveDuration);
        Serial.println(F(" ms)..."));
      }
      digitalWrite(PIN_PUMP_RELAY, LOW); // Sicherstellen, dass die Pumpe aus ist
      digitalWrite(PIN_PUMP_TASTER_LAMP, LOW);
      digitalWrite(PIN_LEDS_TOWER, LOW);
    }
    break;

  case MOVING:
    // Sicherheitscheck: Glas wurde während der Fahrt entfernt
    if (queue.isEmpty() || queue.getNextPosition() != _currentTarget) {
      Serial.println(F("[RUN] Abbruch: Glas waehrend der Fahrt entfernt."));
      _currentTarget = -1;
      state = IDLE;
      break;
    }
    // Sobald die exakte Fahrzeit für den Weg abgelaufen ist:
    if (millis() - stateStartTime >= _expectedMoveDuration) {
      Serial.println(F("[RUN] Zielposition erreicht, Servo abkoppeln & Pumpe starten."));
      Serial.print(F("[DEBUG] Erreichte Position: "));
      Serial.println(_currentTarget);

      // Servo an Zielposition sofort abkoppeln & Signalleitung auf LOW zwingen -> 100% still!
      detach();

      int8_t spotIdx = getSpotIndex((uint8_t)_currentTarget);
      if (spotIdx >= 0) {
        led.setColor(spotIdx + 1,
                     CRGB::Yellow); // LED gelb leuchten lassen (wird befüllt)
        led.setWave(spotIdx + 1, true); // Wellenanimation starten
      } else {
        Serial.println(F("[RUN] Warnung: Position unbekannt."));
      }

      digitalWrite(PIN_PUMP_RELAY, HIGH); // Pumpe starten
      digitalWrite(PIN_LEDS_TOWER, HIGH);  // LED Tower während des Befüllens einschalten
      stateStartTime = millis(); // Startzeit des Abfüllvorgangs speichern
      state = WAITING_PUMP;
    }
    break;

  case WAITING_PUMP:
    // Sicherheitscheck: Glas wurde während des Pumpens entfernt
    if (queue.isEmpty() || queue.getNextPosition() != _currentTarget) {
      digitalWrite(PIN_PUMP_RELAY, LOW); // Pumpe sofort stoppen!
      digitalWrite(PIN_LEDS_TOWER, LOW);  // LED Tower ausschalten
      Serial.println(F("[RUN] Abbruch: Glas waehrend des Pumpens entfernt! "
                       "Pumpe gestoppt."));
      _currentTarget = -1;
      state = IDLE;
      break;
    }
    // Warten, bis die vordefinierte Pumpzeit (WAIT_TIME_PUMP) abgelaufen ist
    if (millis() - stateStartTime >= WAIT_TIME_PUMP) {
      digitalWrite(PIN_PUMP_RELAY, LOW); // Pumpe stoppen
      digitalWrite(PIN_LEDS_TOWER, LOW);  // LED Tower ausschalten
      Serial.println(F("[RUN] Pumpzeit abgelaufen, Pumpe deaktiviert."));

      int8_t spotIdx = getSpotIndex((uint8_t)_currentTarget);
      if (spotIdx >= 0) {
        led.setColor(spotIdx + 1,
                     CRGB::Green); // LED grün leuchten lassen (fertig befüllt)
        led.setWave(spotIdx + 1, false); // Wellenanimation stoppen
      } else {
        Serial.println(F("[RUN] Warnung: Position unbekannt."));
      }
      state = WAITING_NEXT;
    }
    break;

  case WAITING_NEXT:
    // Wenn das Glas während des Abtropfens entfernt wurde, sofort weiter
    if (queue.isEmpty() || queue.getNextPosition() != _currentTarget) {
      Serial.println(F(
          "[RUN] Glas entfernt waehrend Abtropfzeit, ueberspringe Wartezeit."));
      _currentTarget = -1;
      state = IDLE;
      break;
    }
    // Gesamte Wartezeit abwarten, damit Flüssigkeit abtropfen kann (WAIT_TIME
    // abzüglich der Pumpzeit)
    if (millis() - stateStartTime >= WAIT_TIME) {
      Serial.println(
          F("[RUN] Gesamte Wartezeit abgelaufen, naechste Position."));
      Serial.print(F("[DEBUG] Vor Queue.popFront() – Position: "));
      Serial.println(_currentTarget);

      // Den soeben befüllten Becher aus der Warteschlange entfernen
      queue.popFront();
      _currentTarget = -1;
      state = IDLE; // Bereit für den nächsten Befüllvorgang
    }
    break;

  case MOVING_TO_ZERO:
    // Warten, bis Parkposition erreicht ist
    if (millis() - stateStartTime >= _expectedMoveDuration) {
      detach(); // Stromlos schalten & Pin auf LOW halten
      Serial.println(F("[MOVE] Servo in Parkposition angekommen und abgekoppelt."));
      state = IDLE;
    }
    break;
  }
}

void Move::toZero() {
  if (_currentArmAngle != SERVO_PARK_POS) {
    uint16_t diff = abs(_currentArmAngle - SERVO_PARK_POS);
    _expectedMoveDuration = (uint16_t)(((unsigned long)diff * SERVO_SPEED_TIME) / 180) + 50;
    _currentArmAngle = SERVO_PARK_POS;
    attach(PIN_SERVO);
    _servo1.write(SERVO_PARK_POS); // Parkposition (z. B. 170 Grad) anfahren
    _isParking = true;
    _parkStartTime = millis();
  }
}

void Move::detach() {
  _servo1.detach(); // MobaTools-Signal stoppen
  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW); // Feste 0V-Masse auf Signalleitung erzwingen (verhindert Einstreuungen/Rattern)
}

void Move::detachIfIdle() {
  if (_servo1.attached()) {
    if (_isParking) {
      if (millis() - _parkStartTime >= _expectedMoveDuration) {
        _isParking = false;
        detach();
        Serial.println(F("[MOVE] Servo in Parkposition angekommen und abgekoppelt."));
      }
    } else {
      detach();
      Serial.println(F("[MOVE] Servo im Leerlauf abgekoppelt."));
    }
  }
}

void Move::attach(uint8_t pin) {
  _servo1.attach(pin); // Servo wieder an Pin koppeln
  _servo1.setSpeedTime(SERVO_SPEED_TIME); // Fahrzeit für 180 Grad festlegen
}

void Move::begin() {
  pinMode(PIN_PUMP_RELAY, OUTPUT);
  pinMode(PIN_PUMP_TASTER, INPUT_PULLUP);
  pinMode(PIN_PUMP_TASTER_LAMP, OUTPUT);
  pinMode(PIN_LEDS_TOWER, OUTPUT);
  digitalWrite(PIN_PUMP_RELAY, LOW); // Sicherstellen, dass das Relais zu Beginn aus ist
  digitalWrite(PIN_PUMP_TASTER_LAMP, LOW);
  digitalWrite(PIN_LEDS_TOWER, LOW);

  _currentArmAngle = SERVO_PARK_POS;

  // Servo initial sicher abkoppeln und Signalleitung auf 0V legen
  detach();
}

int8_t Move::getSpotIndex(uint8_t pos) const {
  // Sucht den Stellplatz-Index (0 bis NUM_SPOTS-1) für eine gegebene
  // Servoposition
  for (uint8_t i = 0; i < NUM_SPOTS; i++) {
    if (SERVO_POS[i] == pos) {
      return i;
    }
  }
  return -1;
}