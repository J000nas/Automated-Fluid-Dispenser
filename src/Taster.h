//
// Created by Jonas Will on 09.05.25.
//

#ifndef TASTER_H
#define TASTER_H

#include <Arduino.h>
#include "config.h"

/* --Funktion zum Erkennen vom Status vom Start Taster */
bool TasterStart();
/* --Funktion für das Blinken */
bool toggleInInterval(unsigned long interval);

#endif //TASTER_H
