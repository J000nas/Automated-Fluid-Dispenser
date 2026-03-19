#include "Queue.h"
#include <Arduino.h>

/* --Konstruktor */
Queue::Queue(int size) {
    _size = size;
    _queueSize = 0;
    _millisLastUpdate = 0;
    _positionQueue = new int[_size];
    for (int i = 0; i < _size; i++) {
        _positionQueue[i] = 0;
    }
}

/* --Destruktor */
Queue::~Queue() {
    delete[] _positionQueue;
}

/* --Methode zum Hinzufügen von Positionen in der Warteschlange */
bool Queue::addToQueue(int position) {
    for (int i = 0; i < _queueSize; i++) {
        if (_positionQueue[i] == position) {
            return false;  // Schon drin
        }
    }

    if (_queueSize < _size) {
        _positionQueue[_queueSize++] = position;
        return true;
    }

    return false;  // Voll
}

/* --Methode zum Entfernen von Positionen in der Warteschlange */
void Queue::removeFromQueue(int position) {
    for (int i = 0; i < _queueSize; i++) {
        if (_positionQueue[i] == position) {
            for (int j = i; j < _queueSize - 1; j++) {
                _positionQueue[j] = _positionQueue[j + 1];
            }
            _queueSize--;
            _positionQueue[_queueSize] = 0;
            break;
        }
    }
}

/* --Methode zum Anzeigen der Warteschlange */
void Queue::printQueue(){

    unsigned long currentMillis = millis();
    const long interval = 1000;      // Interval in Millisekunden

    // Aktualisiert die Anzeige alle 500 Millisekunden
    if (currentMillis - _millisLastUpdate >= interval) {
        _millisLastUpdate = currentMillis;

        Serial.println("Warteschlange: ");
        for (int i = 0; i < _queueSize; i++) {
            Serial.print("Position ");
            Serial.print(i+1);
            Serial.print(" ");
            Serial.println(_positionQueue[i]);
        }
        Serial.println();
    }

}

/* --Methode zum Ausgeben der ersten Positon in der Warteschlange */
int Queue::getNextPosition() const {
    if (_queueSize > 0) {
        return _positionQueue[0];
    }
    return -1; // -1 bedeutet: keine Position vorhanden
}

void Queue::popFront() {
    if (_queueSize == 0) return;

    for (int i = 0; i < _queueSize - 1; i++) {
        _positionQueue[i] = _positionQueue[i + 1];
    }
    _queueSize--;
}
