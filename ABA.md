## Ausgangslage:

Es gibt schon viele Apps, mit denen man beim Training seine Übungen und Wiederholungen manuell eintragen kann. Unsere App soll mit den intelligenten Handschuhen diese Eintragungen automatisch machen und ein Live-Feedback für die Form des Trägers liefern.

## Untersuchungsanliegen der individuellen Themenstellungen:

Daniel Jones: Programmierung der Microcontrollern, welche auf den Handschuhen befestigt werden und das Labeln und Generieren der KI mit Edge Impulse. Nebenbei soll auch ein 3D-gedrucktes Gehäuse für die Nicla Sense MEs und die Akkus gefertigt werden. 

Kevin Silajdzic: Erstellung einer App mithilfe von Flutter in Android Studio und Verbindung dieser App mit einem Nicla Sense ME über BLE. Die KI wird dann nachher implementiert.

Insgesamt erfordert das Projekt eine gemeinsame Zielsetzung, da Frontend und Backend eng zusammenarbeiten müssen.

## Zielsetzung:

Am Ende soll es ein Paar Handschuhe geben, welche man beim Training anzieht und über BLE mit dem Handy verbindet. In der App soll man die Übung auswählen und die App zählt automatisch die Wiederholungen mit und bewertet die Form des Trägers, welche am Ende einer Übung mit Tipps verbessert wird. Es soll auch festgehaltewerden, wie man sich mit der Zeit in individuellen Übungen verbessert hat.

## Geplantes Ergebnis der individuellen Themenstellungen:

Daniel Jones: Der Code soll die Bewegungsdaten von einem Microcontroller zum anderen senden und mit der KI in verschiedene Wiederholungen teilen und die Form mit der richtigen Form vergleichen und mögliche Fehler erkennen. Die relevanten Daten sollen dann per BLE an das Handy geschickt werden.

Kevin Silajdzic: In der App soll man ein Trainingsset starten können und für dieses Training Übungen auswählen können. Dann startet man das Training und die KI trackt die Bewegungen mit und gibt nach der Übung Feedback zur Ausführung. Man kann auch seine vergangenen Trainings wieder aufrufen und die gespeicherten Statistiken ansehen.