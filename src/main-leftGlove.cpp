#include "Arduino.h"
#include "Nicla_System.h"
#include "ArduinoBLE.h"
#include "Arduino_BHY2.h"
#include "Bench_Press_v2_inferencing.h"

const int MAX_SAMPLES = 27;
const int AXES_PER_SAMPLE = 12;
const int SHIFT_STEP = 4;
const int SAMPLES_TO_KEEP = MAX_SAMPLES - SHIFT_STEP;
const unsigned long SAMPLE_INTERVAL_MS = 100;

float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
float label_sums[EI_CLASSIFIER_LABEL_COUNT];

const int READ_OFFSET = SHIFT_STEP * AXES_PER_SAMPLE;
const size_t BYTES_TO_KEEP = SAMPLES_TO_KEEP * AXES_PER_SAMPLE * sizeof(float);

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

BLEService gloveService("12345678-1234-5678-1234-56789abcdef0");
BLECharacteristic movementChar("12345678-1234-5678-1234-56789abcdef1", BLERead | BLENotify, 24);

BLEDevice rightPeripheral;
BLECharacteristic rightDataChar;

float* handleRightValues();
float* handleLeftValues();
void scanForRightHand();
void fillFeatures(float* leftValues, float* rightValues, int index);
void classifyBenchPress();

void setup() {
    Serial.begin(115200);
    nicla::begin();
    nicla::leds.begin();

    BHY2.begin();
    accel.begin();
    gyro.begin();

    if (!BLE.begin()) {
        while (1);
    }

    BLE.setDeviceName("LeftGlove");
    BLE.setLocalName("LeftGlove");
    BLE.setAdvertisedService(gloveService);
    gloveService.addCharacteristic(movementChar);
    BLE.addService(gloveService);
    BLE.advertise();

    Serial.println("System initialized.");
}

void loop() {
    if (!rightPeripheral || !rightPeripheral.connected()) {
        nicla::leds.setColor(blue); 
        Serial.println("RightGlove not connected. Scanning...");
        BLE.scan(); 
        scanForRightHand();
        nicla::leds.setColor(green); 
    }

    BLEDevice central = BLE.central();

    if (central) {
        Serial.print("📱 App Connected: ");
        Serial.println(central.address());
        
        bool start_command = true; // Platzhalter
        int total_reps = 10;       // Platzhalter

        while (central.connected()) {
            if (!rightPeripheral.connected()) {
                Serial.println("RightGlove lost during session!");
                break; 
            }

            if (start_command) {
                for (int i = 0; i < total_reps; i++) {
                    if (!central.connected() || !rightPeripheral.connected()) break;

                    bool end_of_rep = false; 
                    int measurement_count = 0;
                    memset(features, 0, sizeof(features));
                    memset(label_sums, 0, sizeof(label_sums));
                    static unsigned long lastSampleTime = 0;

                    while (end_of_rep == false) {
                        if (!central.connected() || !rightPeripheral.connected()) break;

                        unsigned long now = millis();

                        if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
                            lastSampleTime = now;

                            float* rightValues = handleRightValues();
                            float* leftValues = handleLeftValues();

                            if (rightValues != nullptr && leftValues != nullptr) {
                                fillFeatures(leftValues, rightValues, measurement_count);
                                measurement_count++;

                                if (measurement_count >= MAX_SAMPLES) {
                                    classifyBenchPress();
                                    measurement_count = SAMPLES_TO_KEEP;
                                }
                            }

                            // Ende der Wiederholung erkennen
                        
                        }
                    }

                    classifyBenchPress();
                    int best_label_idx = 0;
                    for (size_t j = 1; j < EI_CLASSIFIER_LABEL_COUNT; j++) {
                        if (label_sums[j] > label_sums[best_label_idx]) {
                            best_label_idx = j;
                        }
                    }
                    Serial.print("Rep finished: "); Serial.println(best_label_idx);
                }
                start_command = false; 
            }
        }
        Serial.println("⚠️ App disconnected or Partner lost.");
        nicla::leds.setColor(red);
        BLE.advertise(); 
    }
}

void scanForRightHand() {
    while (true) {
        rightPeripheral = BLE.available();
        if (rightPeripheral) {
            if (rightPeripheral.localName() == "RightGlove") {
                Serial.println("Found RightGlove!");
                if (rightPeripheral.connect()) {
                    rightPeripheral.discoverAttributes();
                    rightDataChar = rightPeripheral.service("180C").characteristic("2A58");
                    if (rightDataChar) {
                        BLE.stopScan();
                        break;
                    }
                }
            }
        }
        delay(10);
    }
}

float* handleRightValues() {
    static float rv[6];
    byte buffer[24];
    if (rightDataChar && rightDataChar.readValue(buffer, 24)) {
        memcpy(rv, buffer, 24);
        return rv;
    }
    return nullptr;
}

float* handleLeftValues() {
    BHY2.update();
    static float lv[6];
    lv[0] = accel.x(); lv[1] = accel.y(); lv[2] = accel.z();
    lv[3] = gyro.x();  lv[4] = gyro.y();  lv[5] = gyro.z();
    return lv;
}

void fillFeatures(float* leftValues, float* rightValues, int index) {
    int start_pos = index * AXES_PER_SAMPLE;
    for (size_t i = 0; i < 6; i++) {
        features[start_pos + i] = leftValues[i];
        features[start_pos + i + 6] = rightValues[i];
    }
}

void classifyBenchPress() {
    signal_t signal;
    numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    ei_impulse_result_t result = { 0 };
    if (run_classifier(&signal, &result, false) == EI_IMPULSE_OK) {
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            label_sums[i] += result.classification[i].value;
        }
    }
    memmove(features, features + READ_OFFSET, BYTES_TO_KEEP);
}