#include "AnalogLimit.h"
#include <api/Common.h>

AnalogLimit::AnalogLimit() {
  // Initialisiert alle Sensoren mit einem konservativen Standardwert (888).
  // Dies verhindert Fehltriggerungen, falls vor der Kalibrierung ausgelesen
  // wird.
  for (int i = 0; i < NUM_SPOTS; i++) {
    limits[i] = 888;
  }
}

void AnalogLimit::calibrate() {
  // Führt eine Nullpunktkalibrierung für alle Stellplätze durch.
  // Es wird davon ausgegangen, dass sich beim Start KEINE Becher auf den
  // Stellplätzen befinden.
  for (int i = 0; i < NUM_SPOTS; i++) {
    int temp = 0;

    // 10 Messungen durchführen, um Rauschen zu minimieren
    for (int j = 0; j < 10; j++) {
      temp += analogRead(PIN_SENSORS[i]);
      delay(10);
    }

    // Berechnet den Mittelwert der unbeladenen Messungen
    // und zieht einen Puffer von 30 Einheiten ab, um die Schaltschwelle
    // festzulegen. Ein Becher wird erkannt, wenn der gemessene Analogwert UNTER
    // dieser Schwelle liegt.
    limits[i] = (temp / 10) - 30;
  }
}

int AnalogLimit::getValue(int position) const { return limits[position]; }
