#include "Arduino.h"
#include "Nicla_System.h"

void setup() {
  Serial.begin(9600);
  nicla::begin();          // Initialize sensors, power management, etc.
  nicla::leds.begin();     // Initialize RGB LED
  Serial.println("Nicla Sense ME - Ladeanzeige gestartet");
}

void loop() {
  bool usb = !nicla::_pmic.runsOnBattery(BQ25120A_ADDRESS);
  bool full = nicla::getBatteryChargeLevel() == BatteryChargeLevel::Full;
  bool charging = nicla::getOperatingStatus() == OperatingStatus::Charging;

  if(usb){
    if(full){
      nicla::leds.setColor(green);  // Grün, wenn voll geladen
      nicla::disableCharging();
      Serial.println("Batterie voll geladen.");
    } else if(charging){
      nicla::leds.setColor(white); // Weiß, wenn am Laden
      Serial.println("Batterie wird geladen...");
    } else {
      nicla::leds.setColor(red);   // Blau, wenn USB verbunden, aber nicht ladend
      nicla::enableCharging(125); // Lade mit 125mA
      Serial.println("USB verbunden, aber nicht ladend.");
    }
  } else {
    nicla::leds.setColor(blue);      // Rot, wenn nur Batterie
    Serial.println("Batteriebetrieb.");
  }
  
    delay(10000); // Aktualisiere alle 10 Sekunden
}
