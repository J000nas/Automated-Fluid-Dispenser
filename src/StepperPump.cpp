#include "StepperPump.h"

// ============================================================================
// Konstruktor
// ============================================================================
StepperPump::StepperPump(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin)
    : _stepper(AccelStepper::DRIVER, stepPin, dirPin), _enablePin(enablePin),
      _state(State::IDLE), _currentSpeed(0.0f),
      _startTimeMs(0), _durationMs(0),
      _retractStartMs(0), _lastSpeedUpdateMs(0) {}

// ============================================================================
// begin() - Initialisiert Hardware-Pins und Treiber-Parameter
// ============================================================================
void StepperPump::begin() {
  pinMode(_enablePin, OUTPUT);
  setDriver(false); // TMC2209 EN = HIGH (Zunächst stromlos/kühl)

  _stepper.setPinsInverted(PUMP_INVERT_DIR, false, false);
  _stepper.setMaxSpeed(MAX_SPEED);
  _stepper.setMinPulseWidth(MIN_PULSE_WIDTH);
  _stepper.setSpeed(0.0f);
  _currentSpeed = 0.0f;
}

// ============================================================================
// start() - Startet zeitgesteuerten Dosiervorgang (Abfüllautomatik)
// ============================================================================
void StepperPump::start(uint32_t durationMs) {
  if (durationMs == 0) {
    stop();
    return;
  }

  _durationMs = durationMs;
  _startTimeMs = millis();
  _lastSpeedUpdateMs = 0;
  _state = State::FORWARD;
  _currentSpeed = START_SPEED;

  setDriver(true); // Treiber aktivieren
  _stepper.setSpeed(START_SPEED);
}

// ============================================================================
// startContinuous() - Startet manuelles Dauerpumpen (z. B. Spültaster)
// ============================================================================
void StepperPump::startContinuous() {
  _state = State::MANUAL;
  _durationMs = 0;
  _startTimeMs = millis();
  _lastSpeedUpdateMs = 0;
  _currentSpeed = START_SPEED;

  setDriver(true); // Treiber aktivieren
  _stepper.setSpeed(START_SPEED);
}

// ============================================================================
// stop() - Sofortiger Not-Halt ohne Rückzug (Treiber stromlos)
// ============================================================================
void StepperPump::stop() {
  _state = State::IDLE;
  _currentSpeed = 0.0f;
  _stepper.setSpeed(0.0f);
  setDriver(false); // Treiber sofort stromlos schalten
}

// ============================================================================
// stopWithRetract() - Stoppt Vorwärtslauf abrupt und führt Anti-Drip Rückzug aus
// ============================================================================
void StepperPump::stopWithRetract() {
  // Wenn bereits im Stillstand oder schon im Rückzug, nichts tun
  if (_state == State::IDLE || _state == State::RETRACTING) {
    return;
  }

  // Vorwärtsförderung augenblicklich abrupt stoppen und direkt Rückzug starten
  _state = State::RETRACTING;
  _retractStartMs = millis();
  _currentSpeed = -RETRACT_SPEED;
  _stepper.setSpeed(-RETRACT_SPEED);
  setDriver(true); // Sicherstellen, dass der Treiber für den Rückzug aktiv ist
}

// ============================================================================
// isRunning() - Prüft, ob der Motor aktiv ist
// ============================================================================
bool StepperPump::isRunning() const { return _state != State::IDLE; }

// ============================================================================
// setDriver() - Schaltet den TMC2209 Treiber aktiv oder stromlos
// ============================================================================
void StepperPump::setDriver(bool enable) {
  // TMC2209 EN ist Active-LOW (LOW = Bestromt, HIGH = Stromlos/Kühl)
  digitalWrite(_enablePin, enable ? LOW : HIGH);
}

// ============================================================================
// update() - Berechnet Phasensteuerung, Beschleunigungsrampe und Motorschritte
// Muss in jedem Loop-Durchlauf so oft wie möglich aufgerufen werden!
// ============================================================================
void StepperPump::update() {
  if (_state == State::IDLE) {
    return;
  }

  uint32_t nowMs = millis();

  // --------------------------------------------------------------------------
  // 1. Phasensteuerung (FORWARD -> RETRACTING -> IDLE)
  // --------------------------------------------------------------------------
  if (_state == State::FORWARD) {
    uint32_t elapsed = nowMs - _startTimeMs;

    // Sobald die Restzeit für den Anti-Drip Rückzug erreicht ist:
    // Vorwärtslauf abrupt stoppen und sofort entgegengesetzt drehen!
    if (elapsed + RETRACT_MS >= _durationMs || elapsed >= _durationMs) {
      _state = State::RETRACTING;
      _retractStartMs = nowMs;
      _currentSpeed = -RETRACT_SPEED;
      _stepper.setSpeed(-RETRACT_SPEED);
    }
  } else if (_state == State::RETRACTING) {
    if (nowMs - _retractStartMs >= RETRACT_MS) {
      stop(); // Anti-Drip fertig: Motor stoppen und Treiber stromlos schalten
      return;
    }
  }

  // --------------------------------------------------------------------------
  // 2. Kurze Beschleunigungsrampe (fuer FORWARD und MANUAL)
  // --------------------------------------------------------------------------
  if (_state == State::FORWARD || _state == State::MANUAL) {
    // Nur alle 5 ms die Geschwindigkeit nachjustieren (200 Hz Rate)
    // Spart teure Float-Divisionen und entlastet den 16 MHz AVR vollständig
    if (nowMs - _lastSpeedUpdateMs >= 5) {
      _lastSpeedUpdateMs = nowMs;

      uint32_t elapsed = nowMs - _startTimeMs;
      if (elapsed >= RAMP_UP_MS) {
        if (_currentSpeed != MAX_SPEED) {
          _currentSpeed = MAX_SPEED;
          _stepper.setSpeed(MAX_SPEED);
        }
      } else {
        // Lineare Beschleunigung von START_SPEED bis MAX_SPEED über RAMP_UP_MS
        float progress = (float)elapsed / (float)RAMP_UP_MS;
        float newSpeed = START_SPEED + (MAX_SPEED - START_SPEED) * progress;
        _currentSpeed = newSpeed;
        _stepper.setSpeed(newSpeed);
      }
    }
  }

  // --------------------------------------------------------------------------
  // 3. Schrittgenerierung: Takten des Schrittmotors
  // --------------------------------------------------------------------------
  _stepper.runSpeed();
}
