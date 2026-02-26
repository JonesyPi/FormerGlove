#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

// Initialize sensors
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

    // Nicla setup
    nicla::begin();
    nicla::leds.begin();
    nicla::leds.setColor(blue);

    Serial.println("\n=== LeftGlove Booting ===");

    // Start sensors
    Serial.println("Initializing BHY2 sensors...");
    BHY2.begin();
    accel.begin();
    gyro.begin();
    Serial.println("Sensors initialized successfully.");

    // BLE setup
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

    // If a device connects (e.g., app)
    if (central) {
        Serial.print("📱 Connected to central device: ");
        Serial.println(central.address());
        boolean start = false;

        while (central.connected()) {
            float rax = accel.x();
            float ray = accel.y();
            float raz = accel.z();
            float rgx = gyro.x();
            float rgy = gyro.y();
            float rgz = gyro.z();

            // Convert data into byte array (e.g., 4 bytes per value)
            byte data[24];
            memcpy(data, &rax, 4);
            memcpy(data + 4, &ray, 4);
            memcpy(data + 8, &raz, 4);
            memcpy(data + 12, &rgx, 4);
            memcpy(data + 16, &rgy, 4);
            memcpy(data + 20, &rgz, 4);

            // Send data to app via BLE
            movementChar.writeValue(data, sizeof(data));
            delay(50); // 20 Hz send interval
        }

        // Connection disconnected
        Serial.println("⚠️ Central disconnected.");
        nicla::leds.setColor(red);
        BLE.advertise();
        Serial.println("Re-advertising as LeftGlove...");
    }
}