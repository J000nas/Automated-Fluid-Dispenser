#include <Arduino.h>
#include "config.h"       
#include "AnalogLimit.h"
#include "Queue.h"
#include "LedControl.h"
#include "Move.h"
#include "Taster.h"

bool first = true;
bool help_pump = false;

Queue queue(5);
LedControl led(10); // 10 LEDs
Move move;
AnalogLimit analogLimit;

void setup() {
    Serial.begin(9600);

    digitalWrite(PIN_START_TASTER_LAMP, HIGH); 
    pinMode(PIN_START_TASTER, INPUT_PULLUP);   
    pinMode(PIN_START_TASTER_LAMP, OUTPUT);      

    led.ledStart();
    move.begin();
    analogLimit.calibrate();
}

void loop() {
    if (TasterStart()) {
        move.attach(PIN_SERVO);
        
        if (first) {
            first = false;
            led.setAll(CRGB::Green);
            delay(1000);
            led.clear();
        }
        digitalWrite(PIN_START_TASTER_LAMP, LOW);

        move.status(queue, led, analogLimit);
        move.run(queue, led);
        queue.printQueue();

        help_pump = true;
        delay(50);

    } else {
        move.detach();
        first = true;

        if (help_pump) {
            digitalWrite(PIN_PUMP_RELAY, LOW);
            help_pump = false;
            move.toZero();
        }
        
        if (toggleInInterval(BLINK_INTERVAL)) {
            digitalWrite(PIN_START_TASTER_LAMP, LOW);
        } else {
            digitalWrite(PIN_START_TASTER_LAMP, HIGH);
        }

        move.pump();
        led.blink(BLINK_INTERVAL, CRGB::Yellow);
    }
}