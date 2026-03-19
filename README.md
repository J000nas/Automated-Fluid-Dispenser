# 🚜 Automated Fluid Dispenser (Schnapshäcksler)

Ein automatisiertes, kapazitives Getränke-Abfüllsystem im Maßstab 1:32, verpackt in der Optik eines landwirtschaftlichen Feldhäckslers. 

Dieses Projekt kombiniert 3D-Druck, Mikrocontroller-Programmierung (C++/Arduino) und maßstabsgetreuen Modellbau zu einer vollautomatischen "Bar-Maschine". Sobald ein Schnapsglas auf einen der Stellplätze gestellt wird, erkennt das System dies unsichtbar durch das Plastik hindurch, fährt den Auswurf-Arm exakt über das Glas und schenkt automatisch die perfekte Menge ein.

## ✨ Features

* **Unsichtbare Glas-Erkennung:** Kapazitive Sensoren (Kupferband) sind in das 3D-gedruckte Gehäuse integriert und erkennen Gläser zuverlässig durch eine Plastikschicht hindurch.
* **Indirekte LED-Beleuchtung:** Ein maßgeschneidertes "Lichtkammer"-Design mit WS2811 5mm-LEDs sorgt für eine gleichmäßige, hotspot-freie Ausleuchtung der Stellplätze.
* **Präzise Mechanik:** Ein Servo-Motor steuert den Auswurf-Arm (den "Häcksler-Turm") exakt über den jeweiligen Stellplatz.
* **Automatischer Pumpvorgang:** Eine 12V-Pumpe fördert die Flüssigkeit auf den Milliliter genau.

## 📂 Projektstruktur (PlatformIO)

Dieses Projekt wurde mit **VS Code & PlatformIO** erstellt.

* `src/` - Enthält die Hauptlogik (`main.cpp`)
* `include/` - Enthält die Konfigurationsdatei (`config.h`) mit allen Pin-Belegungen und Schwellenwerten.
* `lib/` - Für lokale Bibliotheken.
* `Dokumente/` - 3D-Druck-Dateien (.stl) und Schaltpläne.

