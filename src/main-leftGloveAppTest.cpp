#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

// Sensoren initialisieren
SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

// BLE Service & Characteristic
BLEService gloveService("12345678-1234-5678-1234-56789abcdef0");
BLECharacteristic movementChar("12345678-1234-5678-1234-56789abcdef1", BLERead | BLENotify, 24);

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
    nicla::leds.setColor(yellow);

    while (central.connected()) {
      BHY2.update();

      float ax = accel.x();
      float ay = accel.y();
      float az = accel.z();
      float gx = gyro.x();
      float gy = gyro.y();
      float gz = gyro.z();

      // Daten serialisiert in Byte-Array
      uint8_t buffer[24];
      memcpy(&buffer[0], &ax, 4);
      memcpy(&buffer[4], &ay, 4);
      memcpy(&buffer[8], &az, 4);
      memcpy(&buffer[12], &gx, 4);
      memcpy(&buffer[16], &gy, 4);
      memcpy(&buffer[20], &gz, 4);

      // Senden über BLE
      movementChar.writeValue(buffer, sizeof(buffer));

      // Debug-Ausgabe
      Serial.println("\n--- LeftGlove Sensor Data ---");
      Serial.print("acc_X: "); Serial.print(ax, 3);
      Serial.print(" | acc_Y: "); Serial.print(ay, 3);
      Serial.print(" | acc_Z: "); Serial.println(az, 3);

      Serial.print("gyro_X: "); Serial.print(gx, 3);
      Serial.print(" | gyro_Y: "); Serial.print(gy, 3);
      Serial.print(" | gyro_Z: "); Serial.println(gz, 3);

      Serial.println("Data sent via BLE ✅");
      delay(100); // ~10 Hz
    }

    // Verbindung getrennt
    Serial.println("⚠️ Central disconnected.");
    nicla::leds.setColor(red);
    BLE.advertise();
    Serial.println("Re-advertising as LeftGlove...");
  }
}
