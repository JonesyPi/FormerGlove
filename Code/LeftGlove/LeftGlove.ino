#include <ArduinoBLE.h>
#include <Nicla_System.h>

// Use standard service & characteristic UUIDs for testing
BLEService ledService("180C");  // Simple, standard custom service
BLEByteCharacteristic ledCharacteristic("2A56", BLERead | BLEWrite); // Byte value for on/off

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT);  // Built-in LED on Nicla

  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }

  BLE.setLocalName("FormerGlove");
  BLE.setAdvertisedService(ledService);

  ledService.addCharacteristic(ledCharacteristic);
  BLE.addService(ledService);

  ledCharacteristic.setValue(0);  // Default: LED off

  delay(100);  // Important: Let BLE stack settle before advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  BLE.poll();  // Handle BLE events

  if (ledCharacteristic.written()) {
    byte value = ledCharacteristic.value();

    if (value) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("LED on");
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("LED off");
    }
  }
}