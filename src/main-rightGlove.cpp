#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"

void sendSensorData();
void checkBattery();

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

BLEService sensorValues("180C");
BLECharacteristic movementChar("2A58",  
                            BLERead | BLENotify,
                            24);     // 6 Floats * 4 Bytes = 24 Bytes

void setup() {
  Serial.begin(9600);
  while (!Serial);

  nicla::begin();
  nicla::enableCharging(125);
  nicla::leds.begin();
  nicla::leds.setColor(100, 200, 255); // light blue

  BHY2.begin();

  accel.begin();
  gyro.begin();

  BLE.begin();
  BLE.setLocalName("RightGlove");
  BLE.setAdvertisedService(sensorValues);
  sensorValues.addCharacteristic(movementChar);
  BLE.addService(sensorValues);
  BLE.advertise();

  Serial.println("Sender ready");
}

void loop() {
  static auto printTime = millis();

  BHY2.update();

  if (millis() - printTime >= 1000) {
    printTime = millis();

    sendSensorData();
    checkBattery();
  }
}

void sendSensorData(){
  float ax = accel.x(), ay = accel.y(), az = accel.z();  
  float gx = gyro.x(), gy = gyro.y(), gz = gyro.z();

  byte buffer[24];
  memcpy(&buffer[0], &ax, 4);
  memcpy(&buffer[4], &ay, 4);
  memcpy(&buffer[8], &az, 4);
  memcpy(&buffer[12], &gx, 4);
  memcpy(&buffer[16], &gy, 4);
  memcpy(&buffer[20], &gz, 4);

  movementChar.writeValue(buffer, 24);
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