//
// Created by Jonas Will on 09.05.25.
//

#ifndef MOVE_H
#define MOVE_H

#include "Queue.h"
#include "LedControl.h"
#include "AnalogLimit.h"
#include <Arduino.h>
#include <MobaTools.h>
#include "config.h"

class Move {
public:
    /* --Konstruktor */
    Move();
    /* --Methode zum Abrufen vom Status der Sensoren */
    void status(Queue& queue, LedControl& led, AnalogLimit& analogLimit);
    /* --Methode um zur Position zu fahren */
    void moveToNext(const Queue& queue);
    /* --Funktion um die Pumpe zum Vorpumpen anzusteuern */
    void pump();
    /* --Methode zum Initialisieren des Motors */
    void begin();
    /* --Methode zum Fahren des Motors */
    void run(Queue& queue, LedControl& led);
    /* --Methode um Servo auf Position 0 zu fahren */
    void toZero();

    void attach(int pin);

    void detach();

    void detachIfIdle();


private:
    int getSpotIndex(int pos) const;
    MoToServo _servo1;
};

#endif //MOVE_H
