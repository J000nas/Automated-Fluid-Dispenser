#include "Queue.h"
#include <Arduino.h>

Queue::Queue(int size) {
  _size = size;
  _queueSize = 0;
  _millisLastUpdate = 0;

  // Dynamisches Array zur Speicherung der Servopositionen anlegen
  _positionQueue = new int[_size];
  for (int i = 0; i < _size; i++) {
    _positionQueue[i] = 0; // Zu Beginn alle Plätze nullen
  }
}

Queue::~Queue() {
  delete[] _positionQueue; // Speicher freigeben, um Memory Leaks zu vermeiden
}

bool Queue::addToQueue(int position) {
  // Prüfen, ob sich die gewünschte Position bereits in der Warteschlange
  // befindet
  for (int i = 0; i < _queueSize; i++) {
    if (_positionQueue[i] == position) {
      return false; // Bereits enthalten, nicht nochmals hinzufügen
    }
  }

  // Wenn noch Platz in der Queue ist, am Ende einfügen
  if (_queueSize < _size) {
    _positionQueue[_queueSize++] = position;
    return true;
  }

  return false; // Warteschlange ist voll
}

void Queue::removeFromQueue(int position) {
  // Sucht das Element in der Queue
  for (int i = 0; i < _queueSize; i++) {
    if (_positionQueue[i] == position) {
      // Nachfolgende Elemente um eine Position nach vorne verschieben
      for (int j = i; j < _queueSize - 1; j++) {
        _positionQueue[j] = _positionQueue[j + 1];
      }
      _queueSize--;                   // Größe verringern
      _positionQueue[_queueSize] = 0; // Letzten Eintrag nullen
      break;
    }
  }
}

void Queue::printQueue() {
  unsigned long currentMillis = millis();
  const long interval = 1000; // Intervall der Ausgabe in Millisekunden

  // Nur alle 1000 ms die Warteschlange ausgeben, um die serielle Konsole nicht
  // zu überfluten
  if (currentMillis - _millisLastUpdate >= interval) {
    _millisLastUpdate = currentMillis;

    Serial.println("Warteschlange: ");
    for (int i = 0; i < _queueSize; i++) {
      Serial.print("Position ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(_positionQueue[i]);
    }
    Serial.println();
  }
}

int Queue::getNextPosition() const {
  // Liefert das vorderste Element (Index 0), falls vorhanden
  if (_queueSize > 0) {
    return _positionQueue[0];
  }
  return -1; // -1 signalisiert, dass keine Position vorliegt
}

void Queue::popFront() {
  // Erstes Element entfernen, wenn die Queue nicht leer ist
  if (_queueSize == 0)
    return;

  // Alle verbleibenden Elemente um eine Position nach vorne rücken
  for (int i = 0; i < _queueSize - 1; i++) {
    _positionQueue[i] = _positionQueue[i + 1];
  }
  _queueSize--; // Größe verringern
}
