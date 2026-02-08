#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

// Sensoren
SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

// --- UUIDs des LeftGlove (MUSS mit den Server-Einstellungen übereinstimmen) ---
const char* LEFT_SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0";
// Characteristic UUID, auf die der RightGlove schreiben wird
const char* LEFT_RECV_CHAR_UUID = "12345678-1234-5678-1234-56789abcdef2"; 

BLEDevice leftPeripheral;         // Referenz zum verbundenen LeftGlove Gerät
BLECharacteristic leftRecvChar;   // Referenz zur Remote-Schreib-Characteristic

unsigned long lastSend = 0;
// Sende-Rate in Millisekunden (100ms = 10Hz)
const unsigned long SEND_INTERVAL_MS = 100; 
bool connectedToLeft = false;

void connectToLeft(BLEDevice peripheral) {
    Serial.print("Gefundenes Peripheriegerät: "); 
    Serial.println(peripheral.localName());
    
    BLE.stopScan();

    Serial.println("Verbinde...");
    if (!peripheral.connect()) {
        Serial.println("❌ Verbindung fehlgeschlagen");
        BLE.scan(); // Scannen neu starten
        return;
    }

    // Services/Characteristics entdecken
    if (!peripheral.discoverAttributes()) {
        Serial.println("❌ Entdeckung der Attribute fehlgeschlagen");
        peripheral.disconnect();
        BLE.scan();
        return;
    }

    // Charakteristik zur Datenübertragung finden
    leftRecvChar = peripheral.characteristic(LEFT_RECV_CHAR_UUID);
    if (!leftRecvChar) {
        Serial.println("❌ Remote-Empfangs-Characteristic nicht auf LeftGlove gefunden");
        peripheral.disconnect();
        BLE.scan();
        return;
    }

    // Schreibberechtigung prüfen
    if (!(leftRecvChar.properties() & BLEWrite)) {
        Serial.println("❌ Remote-Characteristic ist nicht beschreibbar");
        peripheral.disconnect();
        BLE.scan();
        return;
    }

    leftPeripheral = peripheral; // Gerät speichern
    connectedToLeft = true;
    Serial.println("✅ Erfolgreich mit LeftGlove verbunden und Characteristic gefunden.");
}

void setup() {
    Serial.begin(9600);
    // while (!Serial); // Auskommentiert, um das Booten nicht zu blockieren

    nicla::begin();
    nicla::leds.begin();
    nicla::leds.setColor(green); // Statusanzeige: Scannen/Bereit

    Serial.println("=== RightGlove Booting (BLE Client) ===");

    // BHY2 Sensoren initialisieren
    BHY2.begin();
    accel.begin();
    gyro.begin();

    // BLE als Central (Client) starten
    if (!BLE.begin()) {
        Serial.println("❌ BLE Start fehlgeschlagen!");
        while (1);
    }

    // Startet das Scannen (nach Duplikaten scannen, damit wir kontinuierlich Updates sehen)
    BLE.scan(true); 
    Serial.println("Suche nach LeftGlove...");
}

void loop() {
    // Phase 1: Suche & Verbindung
    if (!connectedToLeft) {
        BLEDevice peripheral = BLE.available();
        if (peripheral) {
            // DEBUG: Zeige JEDES gefundene Gerät an, um zu sehen, ob der LeftGlove überhaupt in Reichweite ist
            Serial.print("Scan: ");
            Serial.print(peripheral.address());
            Serial.print(" | Name: ");
            Serial.print(peripheral.localName());
            Serial.print(" | UUID Service: ");
            Serial.println(peripheral.advertisedServiceUuid());

            // Prüfen: Entweder passt die UUID ODER der Name ist "LeftGlove"
            // (Falls der Server die UUID nicht im Advertising-Paket sendet)
            if (peripheral.hasService(LEFT_SERVICE_UUID) || peripheral.localName() == "LeftGlove") {
                Serial.println(">>> Zielgerät gefunden! Versuche Verbindung...");
                connectToLeft(peripheral);
            }
        }
    } else {
        // Phase 2: Datensendung
        
        // Prüfen, ob die Verbindung noch aktiv ist
        if (!leftPeripheral.connected()) {
            Serial.println("Verbindung zu LeftGlove verloren. Starte Scan neu...");
            connectedToLeft = false;
            leftPeripheral = BLEDevice(); // Gerät freigeben
            nicla::leds.setColor(green); // Statusanzeige: Scannen
            BLE.scan(true);
            return;
        }

        // Periodisches Senden der Sensordaten
        if (millis() - lastSend >= SEND_INTERVAL_MS) {
            lastSend = millis();
            BHY2.update(); // Sensorwerte aktualisieren

            float ax = accel.x(), ay = accel.y(), az = accel.z();
            float gx = gyro.x(), gy = gyro.y(), gz = gyro.z();

            // 6 Float-Werte * 4 Bytes/Float = 24 Bytes
            uint8_t buffer[24]; 
            
            // Konvertiere Float-Werte in Bytestrom (Little Endian, Standard für BLE)
            // Dies ist effizienter als String-Konvertierung
            memcpy(&buffer[0], &ax, 4);
            memcpy(&buffer[4], &ay, 4);
            memcpy(&buffer[8], &az, 4);
            memcpy(&buffer[12], &gx, 4);
            memcpy(&buffer[16], &gy, 4);
            memcpy(&buffer[20], &gz, 4);

            // Schreibe die 24 Bytes auf die Characteristic des LeftGlove
            bool ok = leftRecvChar.writeValue(buffer, sizeof(buffer));
            
            if (!ok) {
                Serial.println("⚠️ Schreibvorgang fehlgeschlagen, versuche Neuverbindung...");
                leftPeripheral.disconnect();
                connectedToLeft = false;
                nicla::leds.setColor(green); // Statusanzeige: Scannen
                BLE.scan(true);
            } else {
                // Serial.println("Sent sensor data to LeftGlove ✅"); 
                // Für bessere Performance im Betrieb auskommentierbar
            }
        }
    }
}