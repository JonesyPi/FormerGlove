#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

// Sensoren initialisieren
SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

// BLE Service & Characteristic
BLEService gloveService("12345678-1234-5678-1234-56789abcdef0");
BLECharacteristic movementChar(
    "12345678-1234-5678-1234-56789abcdef1",
    BLERead | BLENotify,
    24
);

void setup() {
    Serial.begin(9600);
    while (!Serial);

    // Nicla Setup
    nicla::begin();
    nicla::leds.begin();
    nicla::leds.setColor(blue);

    Serial.println("\n=== LeftGlove Booting ===");

    // Sensoren starten
    Serial.println("Initializing BHY2 sensors...");
    BHY2.begin();
    accel.begin();
    gyro.begin();
    Serial.println("Sensors initialized successfully.");

    // BLE Setup
    Serial.println("Starting BLE...");
    if (!BLE.begin()) {
        Serial.println("❌ BLE start failed!");
        while (1);
    }

    BLE.setDeviceName("LeftGlove");
    BLE.setLocalName("LeftGlove");
    BLE.setAdvertisedService(gloveService);
    gloveService.addCharacteristic(movementChar);
    BLE.addService(gloveService);
    BLE.advertise();

    nicla::leds.setColor(green);
    Serial.println("✅ BLE advertising started as 'LeftGlove'.");
    Serial.println("Waiting for connection from app...");
}

void loop() {
    BLEDevice central = BLE.central();

    // Wenn sich ein Gerät verbindet (z. B. App)
    if (central) {
        Serial.print("📱 Connected to central device: ");
        Serial.println(central.address());
        boolean start = false;

        while (central.connected()) {
            // Start von Übung von App kontinuierlich abfragen
            if (start == true){
                // Übung & Wiederholungen von App abfragen
                int reps;
                for (int i = 0; i < reps; i++) {
                    float rax = accel.x();
                    float ray = accel.y();
                    float raz = accel.z();
                    float rgx = gyro.x();
                    float rgy = gyro.y();
                    float rgz = gyro.z();

                    // Daten in Byte-Array umwandeln (z.B. 4 Bytes pro Wert)
                    byte data[24];
                    memcpy(data, &rax, 4);
                    memcpy(data + 4, &ray, 4);
                    memcpy(data + 8, &raz, 4);
                    memcpy(data + 12, &rgx, 4);
                    memcpy(data + 16, &rgy, 4);
                    memcpy(data + 20, &rgz, 4);

                    // Daten über BLE senden
                    movementChar.writeValue(data, sizeof(data));
                }
            }
        }

        // Verbindung getrennt
        Serial.println("⚠️ Central disconnected.");
        nicla::leds.setColor(red);
        BLE.advertise();
        Serial.println("Re-advertising as LeftGlove...");
    }
}
