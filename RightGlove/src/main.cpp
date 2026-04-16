#include "Arduino.h"
#include "Nicla_System.h"
#ifdef t_2_10s
  #undef t_2_10s
#endif
#ifdef T1
  #undef T1
#endif
#ifdef T2
  #undef T2
#endif
#ifdef T4
  #undef T4
#endif
#ifdef T8
  #undef T8
#endif

#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"
#include "Bench_Press_State_inferencing.h"

void sendSensorData();

float rightValues[6];

const int AXES_PER_SAMPLE = 12;

const int MAX_SAMPLES_BP_STATE = 6;
const int SHIFT_STEP_BP_STATE = 3;
const int SAMPLES_TO_KEEP_BP_STATE = MAX_SAMPLES_BP_STATE - SHIFT_STEP_BP_STATE;

float features_bp_state[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

const int READ_OFFSET_BP_STATE = SHIFT_STEP_BP_STATE  * AXES_PER_SAMPLE;
const size_t BYTES_TO_KEEP_BP_STATE = SAMPLES_TO_KEEP_BP_STATE * AXES_PER_SAMPLE * sizeof(float);

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

BLEService sensorValues("180C");

BLECharacteristic movementChar("2A58",
                               BLERead | BLENotify,
                               24); // 6 floats * 4 bytes = 24 bytes

BLECharacteristic controlChar("2A59",
                              BLEWrite | BLEWriteWithoutResponse,
                              1); // e.g. 1 byte control signal

BLECharacteristic classificationChar("2A5A",
                                     BLERead | BLEWrite | BLENotify,
                                     25); // 24 bytes features + 1 byte state

// refactored helpers
void checkAndSendSamples();
void handleControlChar();
void handleClassificationWrite();
void fillFeatures_bp_state(float* leftValues, float* rightValues, int index);
boolean classify_bp_state();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting RightGlove...");

  nicla::begin();
  nicla::leds.begin();
  nicla::leds.setColor(100, 200, 255); // light blue

  BHY2.begin();

  accel.begin();
  gyro.begin();

  BLE.begin();
  BLE.setLocalName("RightGlove");
  BLE.setAdvertisedService(sensorValues);
  sensorValues.addCharacteristic(movementChar);
  sensorValues.addCharacteristic(controlChar);
  sensorValues.addCharacteristic(classificationChar);
  BLE.addService(sensorValues);
  BLE.advertise();

  Serial.println("Sender ready");
}

void loop() {
  BHY2.update();

  // Periodic sampling / sending
  checkAndSendSamples();

  // Handle control input (Start / Shift / Classify)
  handleControlChar();
}

/* ---------------------------
   Implementations
   --------------------------- */

void checkAndSendSamples() {
  // keep lastSampleTime static so interval is preserved across calls
  static unsigned long lastSampleTime = millis();

  if (millis() - lastSampleTime >= 50) {
    lastSampleTime = millis();
    sendSensorData();
  }
}

void handleControlChar() {
  if (!controlChar.written()) return;

  byte value;
  controlChar.readValue(value);

  switch (value) {
    case 1:
      // Reset feature buffer
      memset(features_bp_state, 0, sizeof(features_bp_state));
      break;

    case 2:
      // Expecting a features+state packet on classificationChar
      handleClassificationWrite();
      break;

    case 3: {
      // Run classifier on current features and reply with 1/0
      // Note: classificationChar is used for both receiving feature packets
      // and sending back the binary result. This is intentional in your design.
      boolean end = classify_bp_state();
      byte result = end ? 1 : 0;
      classificationChar.writeValue(&result, 1);
      break;
    }

    default:
      break;
  }
}

void handleClassificationWrite() {
  if (!classificationChar.written()) return;

  byte buffer[25];
  classificationChar.readValue(buffer, 25);

  // ---- extract leftValues ----
  float leftValues[6];
  memcpy(leftValues, buffer, 24);

  // ---- measurement_count_state ----
  byte measurement_count_state = buffer[24];

  // fillFeatures_bp_state uses float* leftValues and the global rightValues
  fillFeatures_bp_state(leftValues, rightValues, measurement_count_state);
}

/* ---------------------------
   Original functions (unchanged logic)
   --------------------------- */

void sendSensorData() {
  float ax = accel.x(), ay = accel.y(), az = accel.z();
  float gx = gyro.x(), gy = gyro.y(), gz = gyro.z();

  rightValues[0] = ax;
  rightValues[1] = ay;
  rightValues[2] = az;
  rightValues[3] = gx;
  rightValues[4] = gy;
  rightValues[5] = gz;

  byte buffer[24];
  memcpy(buffer, rightValues, 24);

  // writeValue(..., true) — second parameter (notify) is set to true in original code.
  // This triggers a notification if a central subscribed. Keep as-is for compatibility.
  movementChar.writeValue(buffer, 24, true);
}

void fillFeatures_bp_state(float* leftValues, float* rightValues, int index) {
  int start_pos = index * AXES_PER_SAMPLE;
  for (size_t i = 0; i < 6; i++) {
    features_bp_state[start_pos + i] = leftValues[i];
    features_bp_state[start_pos + i + 6] = rightValues[i];
  }
}

boolean classify_bp_state() {
  signal_t signal;
  numpy::signal_from_buffer(features_bp_state, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  ei_impulse_result_t result = { 0 };
  if (run_classifier(&signal, &result, false) == EI_IMPULSE_OK) {
    memmove(features_bp_state, features_bp_state + READ_OFFSET_BP_STATE, BYTES_TO_KEEP_BP_STATE);
    // classification[1] corresponds to your "state" class — threshold used as in original code
    if (result.classification[1].value > 0.7) return true;
    else return false;
  }
  return false;
}