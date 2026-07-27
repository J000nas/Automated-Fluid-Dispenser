#pragma once

/**
 * @class Queue
 * @brief Verwaltet eine First-In-First-Out (FIFO) Warteschlange für die
 * Servopositionen der zu befüllenden Gläser.
 *
 * Die Klasse verwendet ein dynamisch alloziertes Array, um die Reihenfolge der
 * Stellplätze, auf denen ein Glas erkannt wurde, zu verwalten. Sie stellt
 * sicher, dass kein Stellplatz doppelt eingetragen wird und dass die Gläser in
 * der Reihenfolge ihres Hinstellens befüllt werden (FIFO-Prinzip).
 */
class Queue {
public:
  /**
   * @brief Konstruktor. Erstellt eine Warteschlange mit einer maximalen
   * Kapazität.
   * @param size Maximale Kapazität der Warteschlange (z. B. 5).
   */
  Queue(int size);

  /**
   * @brief Destruktor. Gibt den dynamisch reservierten Speicher für das
   * Positions-Array frei.
   */
  ~Queue();

  /**
   * @brief Fügt eine Servoposition der Warteschlange hinzu.
   *
   * Verhindert doppelte Einträge desselben Stellplatzes.
   *
   * @param position Die hinzuzufügende Servoposition (in Grad).
   * @return true, wenn der Eintrag erfolgreich hinzugefügt wurde, false, wenn
   * er bereits existiert oder die Queue voll ist.
   */
  bool addToQueue(int position);

  /**
   * @brief Entfernt eine Servoposition an einer beliebigen Stelle in der
   * Warteschlange.
   *
   * Wird aufgerufen, wenn ein noch nicht befüllter Becher vorzeitig vom
   * Stellplatz genommen wird. Die nachfolgenden Einträge rücken automatisch
   * nach vorne auf.
   *
   * @param position Die zu entfernende Servoposition (in Grad).
   */
  void removeFromQueue(int position);

  /**
   * @brief Gibt die aktuelle Warteschlange formatiert auf der seriellen
   * Schnittstelle aus.
   *
   * Die Ausgabe erfolgt nicht-blockierend in einem festen Zeitintervall (z. B.
   * alle 1000 ms).
   */
  void printQueue();

  /**
   * @brief Prüft, ob die Warteschlange ihre maximale Kapazität erreicht hat.
   * @return true, wenn voll, andernfalls false.
   */
  bool isFull() const { return _queueSize >= _size; }

  /**
   * @brief Prüft, ob die Warteschlange leer ist.
   * @return true, wenn leer, andernfalls false.
   */
  bool isEmpty() const { return _queueSize == 0; }

  /**
   * @brief Liefert das nächste Element (die erste Position) an der Spitze der
   * Warteschlange, ohne es zu entfernen.
   * @return Die Servoposition an erster Stelle, oder -1, wenn die Warteschlange
   * leer ist.
   */
  int getNextPosition() const;

  /**
   * @brief Gibt die maximale Kapazität (Größe) der Warteschlange zurück.
   * @return Die maximale Anzahl an Elementen.
   */
  int size() const { return _size; }

  /**
   * @brief Gibt die aktuelle Anzahl der Elemente in der Warteschlange zurück.
   * @return Anzahl der belegten Plätze in der Queue.
   */
  int queueSize() const { return _queueSize; }

  /**
   * @brief Entfernt das erste Element (pop) an der Spitze der Warteschlange.
   *
   * Wird aufgerufen, wenn ein Abfüllvorgang für den aktuellen Becher
   * abgeschlossen ist. Alle nachfolgenden Elemente rücken um eine Position nach
   * vorne.
   */
  void popFront();

private:
  int *_positionQueue; ///< Zeiger auf das dynamisch allozierte Array zur
                       ///< Speicherung der Servowinkel
  int _size;           ///< Maximale Kapazität der Warteschlange
  int _queueSize;      ///< Aktuelle Anzahl an Einträgen in der Warteschlange
  unsigned long _millisLastUpdate; ///< Letzter Zeitstempel der seriellen
                                   ///< Ausgabe für die Taktbegrenzung
};
