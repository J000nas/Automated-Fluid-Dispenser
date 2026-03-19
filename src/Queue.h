//
// Created by Jonas Will on 08.05.25.
//

#ifndef QUEUE_H
#define QUEUE_H

class Queue {
public:
    /* --Konstruktor */
    Queue(int size);
    /* --Destruktor */
    ~Queue();
    /* --Methode zum Hinzufügen von Positionen in der Warteschlange */
    bool addToQueue(int position);
    /* --Methode zum Entfernen von Positionen in der Warteschlange */
    void removeFromQueue(int position);
    /* --Methode zum Anzeigen der Warteschlange */
    void printQueue();
    /* --Methode zum Anzeigen, ob die Wartschlange voll ist */
    bool isFull() const { return _queueSize >= _size; }
    /* --Methode zum Anzeigen, ob die Wartschlange leer ist */
    bool isEmpty() const { return _queueSize == 0; }
    /* --Methode zum Ausgeben der ersten Positon in der Warteschlange */
    int getNextPosition() const;
    /* --Methode zum Ausgeben der Größen der Warteschlange */
    int size() const {return _size;}

    int queueSize() const {return _queueSize;}
    /* --Methode um die erste Positon der Wartschlange zu löschen */
    void popFront();


private:
    int* _positionQueue;          // Dynamisches Array
    int _size;                    // Maximale Queue-Größe
    int _queueSize;               // Aktuelle Queue-Größe
    unsigned long _millisLastUpdate;
};



#endif //QUEUE_H
