#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

void printLeftValues();
void printRightValues();
void scanForRightHand();
void printLoop();

BLEDevice peripheral;
BLECharacteristic movementChar;

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  nicla::begin();
  nicla::leds.begin();
  nicla::leds.setColor(green);

  Serial.println("Initializing BLE...");
  BLE.begin();

  Serial.println("Initializing BHY2...");
  BHY2.begin();

  accel.begin();
  gyro.begin();

  
  BLE.scan();
  delay(1000);
  Serial.println("Setup complete.");
}

void loop() {
  scanForRightHand();
  
  printLoop();

  peripheral = BLEDevice();
  BLE.scan();
  delay(1000);
}

void printLeftValues() {
  Serial.println("Left Glove:");
  Serial.print("acc_X: "); Serial.print(accel.x()); Serial.print(", ");
  Serial.print("acc_Y: "); Serial.print(accel.y()); Serial.print(", ");
  Serial.print("acc_Z: "); Serial.println(accel.z());

  Serial.print("gyro_X: "); Serial.print(gyro.x()); Serial.print(", ");
  Serial.print("gyro_Y: "); Serial.print(gyro.y()); Serial.print(", ");
  Serial.print("gyro_Z: "); Serial.println(gyro.z());

  Serial.println();
}

void printRightValues() {
  byte buffer[24];
  if (!movementChar.readValue(buffer, 24)) {
    Serial.println("Failed to read from RightGlove.");
    return;
  }

  float ax, ay, az, gx, gy, gz;
  memcpy(&ax, &buffer[0], 4);
  memcpy(&ay, &buffer[4], 4);
  memcpy(&az, &buffer[8], 4);
  memcpy(&gx, &buffer[12], 4);
  memcpy(&gy, &buffer[16], 4);
  memcpy(&gz, &buffer[20], 4);

  Serial.println("Right Glove:");
  Serial.print("acc_X: "); Serial.print(ax); Serial.print(", ");
  Serial.print("acc_Y: "); Serial.print(ay); Serial.print(", ");
  Serial.print("acc_Z: "); Serial.println(az);

  Serial.print("gyro_X: "); Serial.print(gx); Serial.print(", ");
  Serial.print("gyro_Y: "); Serial.print(gy); Serial.print(", ");
  Serial.print("gyro_Z: "); Serial.println(gz);

  Serial.println();
}

void scanForRightHand(){
  if (!peripheral) {
    Serial.println("Scanning for devices...");
    peripheral = BLE.available();

    if (peripheral) {
      Serial.print("Found device: ");
      Serial.println(peripheral.localName());

      if (peripheral.localName() != "RightGlove") {
        Serial.println("Not the right device, ignoring.");
        peripheral = BLEDevice(); 
        return;
      }

      Serial.println("RightGlove found!");

      BLE.stopScan();
    }

    delay(500);
    return;
  }
}

void printLoop(){
  Serial.println("Attempting to connect...");
  if (peripheral.connect()) {
    Serial.println("Connected to RightGlove.");

    Serial.println("Discovering attributes...");
    peripheral.discoverAttributes();

    movementChar = peripheral.characteristic("2A58");
    if (!movementChar) {
      Serial.println("Characteristic not found!");
      peripheral.disconnect();
      return;
    }
    Serial.println("Movement characteristic found.");

    while (peripheral.connected()) {
      BHY2.update();

      printLeftValues();
      printRightValues();

      delay(1000);
    }

    Serial.println("Disconnected from RightGlove.");
  } else {
    Serial.println("Connection failed.");
  }
}