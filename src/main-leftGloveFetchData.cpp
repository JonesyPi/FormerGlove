#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

// Function declarations
float* handleLeftValues();
float* handleRightValues();
void scanForRightHand();
void handleValuesLoop();
void sendViaBLE(float* leftValues, float* rightValues);
void sendToPC(float* leftValues, float* rightValues);

BLEDevice peripheral;
BLECharacteristic movementChar;

BLEService gloveService("12345678-1234-5678-1234-56789abcdef0"); 
BLECharacteristic combinedDataChar("12345678-1234-5678-1234-56789abcdef1", BLERead | BLENotify, 48);

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  nicla::begin();
  nicla::leds.begin();
  nicla::leds.setColor(green);

  Serial.println("Initializing BLE...");
  BLE.begin();

  BLE.setDeviceName("LeftGlove");
  BLE.setLocalName("LeftGlove");

  gloveService.addCharacteristic(combinedDataChar);
  BLE.setAdvertisedService(gloveService);
  BLE.addService(gloveService);
  BLE.advertise();
  Serial.println("Advertising started for LeftGlove.");
  combinedDataChar.writeValue("{}"); 

  Serial.println("BLE service for LeftGlove started.");

  Serial.println("Initializing BHY2...");
  BHY2.begin();

  accel.begin();
  gyro.begin();

  BLE.scan();
  delay(1000);
  scanForRightHand();
  Serial.println("Setup complete.");
}

void loop() {
  handleValuesLoop();
}

void handleValuesLoop(){
  Serial.println("Attempting to connect...");
  if (peripheral.connect()) {
    Serial.println("Connected to RightGlove.");

    Serial.println("Discovering attributes...");
    peripheral.discoverAttributes();

    movementChar = peripheral.service("180C").characteristic("2A58");
    if (!movementChar) {
      Serial.println("Characteristic not found!");
      peripheral.disconnect();
      scanForRightHand();
      return;
    }
    Serial.println("Movement characteristic found.");

    while (peripheral.connected()) {
      BHY2.update(); // Update sensor values

      float* leftValues = handleLeftValues();
      float* rightValues = handleRightValues();
      if (rightValues == nullptr) {
        Serial.println("Skipping this loop iteration due to read failure.");
        continue; 
      }

      sendViaBLE(leftValues, rightValues);  // send via BLE
      sendToPC(leftValues, rightValues);    // send to PC via Serial

      delay(50);
    }

    Serial.println("Disconnected from RightGlove.");
  } else {
    Serial.println("Connection failed.");
  }
  scanForRightHand();
}

float* handleLeftValues() {
  // Retrieve sensor values from the left glove

  float ax = accel.x(), ay = accel.y(), az = accel.z();  
  float gx = gyro.x(), gy = gyro.y(), gz = gyro.z();

  static float leftValues[6]; // Keep array static so that pointer can be returned
  leftValues[0] = ax;
  leftValues[1] = ay;
  leftValues[2] = az;
  leftValues[3] = gx;
  leftValues[4] = gy;
  leftValues[5] = gz;

  Serial.println("Left Glove:");
  Serial.print("acc_X: "); Serial.print(ax); Serial.print(", ");
  Serial.print("acc_Y: "); Serial.print(ay); Serial.print(", ");
  Serial.print("acc_Z: "); Serial.println(az);

  Serial.print("gyro_X: "); Serial.print(gx); Serial.print(", ");
  Serial.print("gyro_Y: "); Serial.print(gy); Serial.print(", ");
  Serial.print("gyro_Z: "); Serial.println(gz);

  Serial.println();

  return leftValues;
}

float* handleRightValues() {
  // Retrieve sensor values from the right glove from the characteristic and convert into float array

  byte buffer[24];
  if (!movementChar.readValue(buffer, 24)) {
    Serial.println("Failed to read from RightGlove.");
    return nullptr;
  }

  float ax, ay, az, gx, gy, gz;
  memcpy(&ax, &buffer[0], 4);
  memcpy(&ay, &buffer[4], 4);
  memcpy(&az, &buffer[8], 4);
  memcpy(&gx, &buffer[12], 4);
  memcpy(&gy, &buffer[16], 4);
  memcpy(&gz, &buffer[20], 4);

  static float rightValues[6]; // Number of sensor axes
  rightValues[0] = ax;
  rightValues[1] = ay;
  rightValues[2] = az;
  rightValues[3] = gx;
  rightValues[4] = gy;
  rightValues[5] = gz;
  
  Serial.println("Right Glove:");
  Serial.print("acc_X: "); Serial.print(ax); Serial.print(", ");
  Serial.print("acc_Y: "); Serial.print(ay); Serial.print(", ");
  Serial.print("acc_Z: "); Serial.println(az);

  Serial.print("gyro_X: "); Serial.print(gx); Serial.print(", ");
  Serial.print("gyro_Y: "); Serial.print(gy); Serial.print(", ");
  Serial.print("gyro_Z: "); Serial.println(gz);

  Serial.println();

  return rightValues;
}

void scanForRightHand(){
  // Checking if the right glove appears in the BLE scan results and connecting to it

  Serial.println("Scanning for RightGlove...");
  while(true){
    peripheral = BLE.available();
    if(peripheral) {
      if (peripheral.localName() == "RightGlove") {
        Serial.println("RightGlove found!");
        BLE.stopScan();
        break;
      }
    }
    delay(10);
  }
}

void sendViaBLE(float* leftValues, float* rightValues) {
  if (!leftValues || !rightValues) return;

  uint8_t buffer[48];  // 12 floats * 4 bytes

  // Go through all sensor axes and pack them into the buffer

  for (int i = 0; i < 6; i++) {
    memcpy(&buffer[i*4], &leftValues[i], 4);
  }

  for (int i = 0; i < 6; i++) {
    memcpy(&buffer[24 + i*4], &rightValues[i], 4);
  }

  combinedDataChar.writeValue(buffer, sizeof(buffer)); 
}

void sendToPC(float* leftValues, float* rightValues) {
  if (!leftValues || !rightValues) return;

  // Go through all sensor axes and output them via Serial

  for (int i = 0; i < 6; i++) {
    Serial.print(leftValues[i], 2);
    Serial.print(",");
  }
  for (int i = 0; i < 5; i++) {
    Serial.print(rightValues[i], 2);
    Serial.print(",");
  }
  Serial.println(rightValues[5], 2); // Last value without comma
}