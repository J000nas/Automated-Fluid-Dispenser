//
// Created by Jonas Will on 13.08.25.
//

#ifndef ANALOGLIMIT_H
#define ANALOGLIMIT_H

#include "config.h"

class AnalogLimit {
public:
    AnalogLimit();

    void calibrate();

    int getValue(int position) const;
private:
    int limits[NUM_SPOTS];


};



#endif //ANALOGLIMIT_H
