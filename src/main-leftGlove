#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

float* handleLeftValues();
float* handleRightValues();
void scanForRightHand();
void handleValuesLoop();
void sendAsJSON(float* leftValues, float* rightValues);
void checkBattery();

BLEDevice peripheral;
BLECharacteristic movementChar;

BLEService gloveService("12345678-1234-5678-1234-56789abcdef0"); 
BLECharacteristic combinedDataChar("12345678-1234-5678-1234-56789abcdef1", BLERead | BLENotify, 300);

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  nicla::begin();
  nicla::enableCharging(125);
  nicla::leds.begin();
  nicla::leds.setColor(green);

  Serial.println("Initializing BLE...");
  BLE.begin();

  BLE.setLocalName("Former Glove");
  BLE.setAdvertisedService(gloveService);
  gloveService.addCharacteristic(combinedDataChar);
  BLE.addService(gloveService);
  combinedDataChar.writeValue("{}"); 

  BLE.advertise();

  Serial.println("BLE service for LeftGlove started.");

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
  
  handleValuesLoop();

  peripheral = BLEDevice();
  BLE.scan();
  checkBattery();
  delay(1000);
}

void handleValuesLoop(){
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

      float* leftValues = handleLeftValues();
      float* rightValues = handleRightValues();
      sendAsJSON(leftValues, rightValues);
      checkBattery();

      delay(1000);
    }

    Serial.println("Disconnected from RightGlove.");
  } else {
    Serial.println("Connection failed.");
  }
}

float* handleLeftValues() {
  float ax = accel.x(), ay = accel.y(), az = accel.z();  
  float gx = gyro.x(), gy = gyro.y(), gz = gyro.z();

  static float leftValues[6] = {ax, ay, az, gx, gy, gz};

  Serial.println("Left Glove:");
    Serial.print("acc_X: "); Serial.print(accel.x()); Serial.print(", ");
    Serial.print("acc_Y: "); Serial.print(accel.y()); Serial.print(", ");
    Serial.print("acc_Z: "); Serial.println(accel.z());

    Serial.print("gyro_X: "); Serial.print(gyro.x()); Serial.print(", ");
    Serial.print("gyro_Y: "); Serial.print(gyro.y()); Serial.print(", ");
    Serial.print("gyro_Z: "); Serial.println(gyro.z());

  Serial.println();

  return leftValues;
}

float* handleRightValues() {
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

  static float rightValues[6] = {ax, ay, az, gx, gy, gz};

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

void sendAsJSON(float* leftValues, float* rightValues){
  StaticJsonDocument<256> doc;

  JsonArray left = doc.createNestedArray("left");
  JsonArray right = doc.createNestedArray("right");

  for (int i = 0; i < 6; i++) {
    left.add(leftValues[i]);
  }

  for (int i = 0; i < 6; i++) {
    right.add(rightValues[i]);
  }

  String output;
  serializeJson(doc, output);

  Serial.println("Sending combined glove data as JSON:");
  Serial.println(output);
  Serial.println();

  combinedDataChar.writeValue(output.c_str());
}

void checkBattery(){
  auto operatingStatus = nicla::getOperatingStatus();
  
  switch (operatingStatus)
  {
  case OperatingStatus::Charging:
    nicla::leds.setColor(yellow);
    break;
  case OperatingStatus::ChargingComplete:
    nicla::leds.setColor(100, 200, 255);
    break;
  default:
    float batteryVoltage = nicla::getCurrentBatteryVoltage();
    if (batteryVoltage < 3.55){
      nicla::leds.setColor(red);
    }
    break;
  }
}