//
// Created by Jonas Will on 13.08.25.
//

#include "AnalogLimit.h"

#include <api/Common.h>

AnalogLimit::AnalogLimit() {
    for (int i=0; i<NUM_SPOTS;i++) {
        limits[i]=888;
    }
}

void AnalogLimit::calibrate() {
    for (int i=0; i<NUM_SPOTS;i++) {
        int temp = 0;
        for (int j=0; j<10; j++) {
            temp += analogRead(PIN_SENSORS[i]);
            delay(10);
        }
        limits[i] = (temp/10)-30;
    }
}

int AnalogLimit::getValue(int position) const {
    return limits[position];
}

