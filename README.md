# FormerGlove

Daniel:

First Test:

Arduino IDE

Board:
Nicla Sense ME

Board Manager:
Arduino Mbed OS Nicla Boards

This code changes the colour of the Nicla's LED periodically, so I used it just to check the Nicla works correctly.


Connecting to Periphials via BLE:

Libraries:
ArduinoBLE

This is a minimalized code to connect the Nicla Sense to a periphial and toggle the built-in LED. I'm using LightBlue on iOS to connect and write a hex value (00 -> LED off; 01 -> LED on)


Testing the Sensors:

Libraries:
Arduino
Arduino_BHY2

Here, I tested the IMU sensor of the Nicla Sense, which includes an accelerometer and a gyroscope. These will be used to measure the movement and rotation of the glove.



