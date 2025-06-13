#include <Arduino.h>
#include "Nicla_System.h"

void setup() {
  nicla::begin();
  nicla::leds.begin();
}

void loop() {
  nicla::leds.setColor(green);
  delay(200);
  nicla::leds.setColor(blue);
  delay(200); 
  nicla::leds.setColor(red);
  delay(200);
  nicla::leds.setColor(yellow);
  delay(200);
  nicla::leds.setColor(magenta);
  delay(200);
  nicla::leds.setColor(cyan);
  delay(200);
}