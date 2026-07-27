#include "CupSensorManager.h"
#include <Wire.h>

CupSensorManager::CupSensorManager() {
  for (uint8_t i = 0; i < NUM_SENSORS; i++) {
    glasErkannt[i] = false;
    differenzenRaw[i] = 0;
    differenzenKompensiert[i] = 0;
  }
}

bool CupSensorManager::begin(uint8_t i2cAddress) {
  // 1. MPR121 Sensor starten
  // begin() setzt standardmäßig eigene Thresholds und schaltet in den Run-Mode
  // (ECR = 0x8F)
  if (!cap.begin(i2cAddress)) {
    return false;
  }

  // 2. Sensor Tuning und Registerkonfiguration
  // Wichtig: Um Konfigurationsregister zu beschreiben, muss der MPR121 im
  // STOP-Mode sein!
  writeRegister(MPR121_ECR, 0x00); // Stop-Mode: Messungen anhalten

  // Auto-Configuration deaktivieren (Manuelle Kalibrierung)
  writeRegister(MPR121_AUTOCONFIG0, 0x00);
  writeRegister(MPR121_AUTOCONFIG1, 0x00);

  // Manuelle Parameter für C = 12.4pF setzen
  // Ziel: Basiswert soll sich bei ca. 750 (in 10-Bit) einpendeln.
  // Formel: Raw = 25 * CDC * CDT -> CDC = 15uA bei CDT = 2us ergibt ca. 750
  // counts.

  // CONFIG1 (0x5C): FFI = 10 (18 samples), CDC = 15uA (0x0F) -> 0b10001111 =
  // 0x8F
  writeRegister(MPR121_CONFIG1, 0x8F);

  // CONFIG2 (0x5D): CDT = 011 (2us Ladezeit), SFI = 01 (6 samples), ESI = 011
  // (8ms) -> 0b01101011 = 0x6B
  writeRegister(MPR121_CONFIG2, 0x6B);

  // Debounce (Entprellung) einstellen
  // DEBOUNCE (0x5B):
  // - Bit 6-4: Release Debounce (Entprellung beim Loslassen) -> 2 Samples (010)
  // - Bit 2-0: Touch Debounce (Entprellung beim Berühren) -> 2 Samples (010)
  // Wert: 0b00100010 = 0x22
  writeRegister(MPR121_DEBOUNCE, 0x22);

  // Baseline-Filtering (Drift-Kompensation) optimieren
  // Wir optimieren die Filter-Parameter, damit der Sensor langsame Drift
  // (Feuchtigkeit, Temp) ausgleicht, aber ein aufgesetztes Glas nicht sofort
  // als "neue Baseline" wegkalibriert.

  // Rising (Elektrode unberührt / Glas weggenommen -> Signal steigt)
  // Wir machen das Ansteigen sehr schnell, damit die Baseline beim Wegnehmen
  // des Glases sofort auf ~750 hochschnellt.
  writeRegister(MPR121_MHDR, 0x01); // Max Half Delta
  writeRegister(MPR121_NHDR, 0x01); // Noise Half Delta
  writeRegister(
      MPR121_NCLR,
      0x01); // Noise Count Limit (sehr schnell anpassen bei steigendem Signal)
  writeRegister(MPR121_FDLR, 0x00); // Filter Delay Limit (sofortige Reaktion)

  // Falling (Glas aufgesetzt -> Signal fällt)
  writeRegister(MPR121_MHDF, 0x01);
  writeRegister(
      MPR121_NHDF,
      0x05); // Größeres Noise-Fenster nach unten, um Rauschen zu tolerieren
  writeRegister(MPR121_NCLF, 0xFF); // Sehr hohes Count Limit -> Baseline
                                    // wandert extrem langsam nach unten!
  writeRegister(MPR121_FDLF, 0x02);

  // Touched (Wenn Glas aktiv erkannt ist)
  // Wir sperren die Drift-Anpassung während ein Glas steht, damit es unbegrenzt
  // erkannt bleibt.
  writeRegister(MPR121_NHDT, 0x00);
  writeRegister(MPR121_NCLT, 0x00);
  writeRegister(MPR121_FDLT, 0x00);

  // ECR (0x5E):
  // - Bit 7-6: CL = 10 (Baseline-Tracking und Kalibrierung aktiv)
  // - Bit 5-4: ELEPROX = 00 (Proximity-Modus aus)
  // - Bit 3-0: ELE_EN = 0101 (Nur ELE0 bis ELE4 aktivieren, also 5 Kanäle statt
  // 12) Wert: 0b10000101 = 0x85
  writeRegister(MPR121_ECR, 0x85);

  return true;
}

void CupSensorManager::update() {
  // 1. Rohdifferenzen aller 5 Sensoren einlesen
  for (uint8_t s = 0; s < NUM_SENSORS; s++) {
    uint8_t pin = SENSOR_PINS[s];
    uint16_t baseline = cap.baselineData(pin);
    uint16_t filtered = cap.filteredData(pin);
    differenzenRaw[s] = (int16_t)baseline - (int16_t)filtered;
  }

  // 2. Asymmetrische Software-Crosstalk-Kompensation
  for (uint8_t s = 0; s < NUM_SENSORS; s++) {
    int16_t linkerNachbarAbzug = 0;
    int16_t rechterNachbarAbzug = 0;

    // Linker Nachbar (Kopplung von links überall ca. 24%)
    if (s > 0) {
      int16_t rawL = differenzenRaw[s - 1];
      if (rawL > 0) {
        linkerNachbarAbzug = rawL * 24 / 100;
      }
    }

    // Rechter Nachbar (Kopplung von rechts: S1 auf S0 ist 55% wegen GND, sonst
    // 24%)
    if (s < NUM_SENSORS - 1) {
      int16_t rawR = differenzenRaw[s + 1];
      if (rawR > 0) {
        uint8_t coeff = (s == 0) ? 55 : 24;
        rechterNachbarAbzug = rawR * coeff / 100;
      }
    }

    int16_t kompensiert =
        differenzenRaw[s] - linkerNachbarAbzug - rechterNachbarAbzug;
    differenzenKompensiert[s] = kompensiert >= 0 ? kompensiert : 0;
  }

  // 3. Zustandserkennung mit Hysterese
  for (uint8_t s = 0; s < NUM_SENSORS; s++) {
    int16_t diff = differenzenKompensiert[s];

    if (!glasErkannt[s]) {
      if (diff >= TOUCH_THRESHOLD) {
        glasErkannt[s] = true;
      }
    } else {
      if (diff < RELEASE_THRESHOLD) {
        glasErkannt[s] = false;
      }
    }
  }
}

bool CupSensorManager::isCupDetected(uint8_t index) const {
  if (index < NUM_SENSORS) {
    return glasErkannt[index];
  }
  return false;
}

int16_t CupSensorManager::getCompensatedDiff(uint8_t index) const {
  // Liefert den fertigen, entkoppelten Signalwert für die
  // Hysterese-Entscheidung
  if (index < NUM_SENSORS) {
    return differenzenKompensiert[index];
  }
  return 0;
}

int16_t CupSensorManager::getRawDiff(uint8_t index) const {
  // Liefert den reinen Kapazitätsabfall vor dem crosstalk-Abzug (wichtig für
  // Debugging)
  if (index < NUM_SENSORS) {
    return differenzenRaw[index];
  }
  return 0;
}

uint8_t CupSensorManager::getSensorPin(uint8_t index) const {
  // Liefert den physischen MPR121-Kanal (0 bis 4) für den logischen
  // Stellplatz-Index
  if (index < NUM_SENSORS) {
    return SENSOR_PINS[index];
  }
  return 255;
}

void CupSensorManager::writeRegister(uint8_t reg, uint8_t value) {
  // Überträgt I2C-Befehl zum Schreiben eines Konfigurationsregisters
  Wire.beginTransmission(0x5A); // Standard I2C-Adresse des Sensors
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}
