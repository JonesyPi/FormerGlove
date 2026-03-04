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
#include "Bench_Press_v3_inferencing.h"

const int AXES_PER_SAMPLE = 12;

const int MAX_SAMPLES_BP_STATE = 6;
const int SHIFT_STEP_BP_STATE = 3;
const int SAMPLES_TO_KEEP_BP_STATE = MAX_SAMPLES_BP_STATE - SHIFT_STEP_BP_STATE;

const int MAX_SAMPLES_BP_FORM = 27;
const int SHIFT_STEP_BP_FORM = 4;
const int SAMPLES_TO_KEEP_BP_FORM = MAX_SAMPLES_BP_FORM - SHIFT_STEP_BP_FORM;
const unsigned long SAMPLE_INTERVAL_MS = 100;

float features_bp_form[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
float label_sums_bp_form[EI_CLASSIFIER_LABEL_COUNT];

const int READ_OFFSET_BP_FORM = SHIFT_STEP_BP_FORM * AXES_PER_SAMPLE;
const size_t BYTES_TO_KEEP_BP_FORM = SAMPLES_TO_KEEP_BP_FORM * AXES_PER_SAMPLE * sizeof(float);

SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);

BLEService gloveService("12345678-1234-5678-1234-56789abcdef0");

BLECharacteristic movementChar("12345678-1234-5678-1234-56789abcdef1",
   BLERead | BLENotify,
   24);

BLECharacteristic rxChar(
    "12345678-1234-5678-1234-56789abcdef2",
    BLEWrite | BLEWriteWithoutResponse,
    4
);

BLECharacteristic labelChar(
    "12345678-1234-5678-1234-56789abcdef3",
    BLERead | BLENotify,
    1
);

BLEDevice rightPeripheral;
BLECharacteristic rightDataChar;
BLECharacteristic controlChar;
BLECharacteristic classificationChar;

// Prototypes
float* handleRightValues();
float* handleLeftValues();
void scanForRightHand();
void fillFeatures_bp_form(float* leftValues, float* rightValues, int index);
void classify_bp_form();

// refactored helpers (same names as before)
void ensureRightConnected();
void handleCentral(BLEDevice central);
void readAppCommand();
void executeWorkout(BLEDevice central);
void performRep(BLEDevice central);
void handleExerciseInit();
void processSample(bool &end_of_rep,
                   int &measurement_count_form,
                   int &measurement_count_state,
                   int &total_running,
                   int &total_end,
                   bool &end);
void sendLabelToApp(int best_label_idx);

// State
bool start_command = false;
int total_reps = 0;
int weight = 0;
int exercise = 0;

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
    gloveService.addCharacteristic(rxChar);
    gloveService.addCharacteristic(labelChar);
    BLE.addService(gloveService);
    BLE.advertise();

    Serial.println("System initialized.");
}

void loop() {
    // Keep sensors updated
    BHY2.update();

    // Ensure connection to partner glove
    ensureRightConnected();

    // Check for app connection (central)
    BLEDevice central = BLE.central();
    if (central) {
        handleCentral(central);
    }
}

/* ---------------------------
   Implementations
   --------------------------- */

void ensureRightConnected() {
    if (!rightPeripheral || !rightPeripheral.connected()) {
        nicla::leds.setColor(blue);
        Serial.println("RightGlove not connected. Scanning...");
        BLE.scan();
        scanForRightHand();
        nicla::leds.setColor(green);
    }
}

void handleCentral(BLEDevice central) {
    Serial.print("📱 App Connected: ");
    Serial.println(central.address());

    while (central.connected()) {
        // If partner disconnected, break and restart scanning
        if (!rightPeripheral.connected()) {
            Serial.println("RightGlove lost during session!");
            break;
        }

        // Read incoming app commands (rxChar)
        readAppCommand();

        // If a start command was received, run the workout sequence
        if (start_command) {
            executeWorkout(central);
            start_command = false;
        }
    }

    Serial.println("⚠️ App disconnected or Partner lost.");
    nicla::leds.setColor(red);
    BLE.advertise();
}

void readAppCommand() {
    if (!rxChar.written()) return;

    byte buffer[4];
    rxChar.readValue(buffer, 4);

    if (buffer[0] == 1) {   // Start-Flag
        start_command = true;
        exercise = buffer[1];
        total_reps = buffer[2];
        weight = buffer[3];
    }
}

void executeWorkout(BLEDevice central) {
    for (int i = 0; i < total_reps; i++) {
        if (!central.connected() || !rightPeripheral.connected()) break;
        performRep(central);
    }
}

void performRep(BLEDevice central) {
    bool end_of_rep = false;
    int measurement_count_form = 0;
    int measurement_count_state = 0;
    int total_running = 0;
    int total_end = 0;
    bool end = false;

    // init for exercise (sends control signal 1)
    handleExerciseInit();

    // inner sampling / inference loop for a single repetition
    while (!end_of_rep) {
        if (!central.connected() || !rightPeripheral.connected()) break;

        processSample(end_of_rep,
                      measurement_count_form,
                      measurement_count_state,
                      total_running,
                      total_end,
                      end);
    }

    // at end of rep, run form classifier pooling and send best label
    int best_label_idx = 0;
    if (exercise == 0) {
        classify_bp_form();
        for (size_t j = 1; j < EI_CLASSIFIER_LABEL_COUNT; j++) {
            if (label_sums_bp_form[j] > label_sums_bp_form[best_label_idx]) {
                best_label_idx = j;
            }
        }
    }

    sendLabelToApp(best_label_idx);
}

void handleExerciseInit() {
    if (exercise != 0) return;

    memset(features_bp_form, 0, sizeof(features_bp_form));
    memset(label_sums_bp_form, 0, sizeof(label_sums_bp_form));

    byte signal = 1;
    controlChar.writeValue(&signal, 1);
}

/* processSample: single timed sampling / messaging / state update
   -- NOTE: logic preserved exactly from your original code
*/
void processSample(bool &end_of_rep,
                   int &measurement_count_form,
                   int &measurement_count_state,
                   int &total_running,
                   int &total_end,
                   bool &end)
{
    static unsigned long lastSampleTime = 0;

    unsigned long now = millis();
    if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
    lastSampleTime = now;

    float* leftValues = handleLeftValues();
    float* rightValues = handleRightValues();

    if (!leftValues || !rightValues) return;

    if (exercise != 0) return;

    // fill sliding window for form classifier
    fillFeatures_bp_form(leftValues, rightValues, measurement_count_form);
    measurement_count_form++;

    if (measurement_count_form >= MAX_SAMPLES_BP_FORM) {
        classify_bp_form();
        measurement_count_form = SAMPLES_TO_KEEP_BP_FORM;
    }

    // send rightValues + measurement_count_state to app via movementChar (25 bytes)
    byte buffer[25];
    memcpy(&buffer[0], &rightValues, 24);
    memcpy(&buffer[24], &measurement_count_state, 1);
    movementChar.writeValue(buffer, 25);

    // notify RightGlove to shift buffer (signal 2)
    byte signal = 2;
    controlChar.writeValue(&signal, 1);

    measurement_count_state++;

    // if enough state samples, request classification from RightGlove (signal 3)
    if (measurement_count_state >= MAX_SAMPLES_BP_STATE) {
        byte signal = 3;
        controlChar.writeValue(&signal, 1);
        measurement_count_state = SAMPLES_TO_KEEP_BP_STATE;

        // wait for classificationChar update from RightGlove
        while (true) {
            if (classificationChar.valueUpdated()) {
                byte result;
                classificationChar.readValue(result);
                end = result == 1;
                break;
            }
        }
    }

    // update counters for end-of-rep voting logic
    if (!end) {
        total_running++;
    } else {
        total_end++;
        if (total_running >= 2 &&
            (float)total_end / (float)(total_running + total_end) > 0.3) {
            end_of_rep = true;
        }
    }
}

void sendLabelToApp(int best_label_idx) {
    uint8_t idx = (uint8_t)best_label_idx;
    labelChar.writeValue(&idx, 1);
}

/* ---------------------------
   Unchanged helper functions (IO)
   --------------------------- */

void scanForRightHand() {
    while (true) {
        rightPeripheral = BLE.available();
        if (rightPeripheral) {
            if (rightPeripheral.localName() == "RightGlove") {
                Serial.println("Found RightGlove!");
                if (rightPeripheral.connect()) {
                    rightPeripheral.discoverAttributes();
                    rightDataChar = rightPeripheral.service("180C").characteristic("2A58");
                    controlChar = rightPeripheral.service("180C").characteristic("2A59");
                    classificationChar = rightPeripheral.service("180C").characteristic("2A5A");
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
    static float lv[6];
    lv[0] = accel.x();
    lv[1] = accel.y();
    lv[2] = accel.z();
    lv[3] = gyro.x();
    lv[4] = gyro.y();
    lv[5] = gyro.z();
    return lv;
}

void fillFeatures_bp_form(float* leftValues, float* rightValues, int index) {
    int start_pos = index * AXES_PER_SAMPLE;
    for (size_t i = 0; i < 6; i++) {
        features_bp_form[start_pos + i] = leftValues[i];
        features_bp_form[start_pos + i + 6] = rightValues[i];
    }
}

void classify_bp_form() {
    signal_t signal;
    numpy::signal_from_buffer(features_bp_form, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    ei_impulse_result_t result = {0};
    if (run_classifier(&signal, &result, false) == EI_IMPULSE_OK) {
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            label_sums_bp_form[i] += result.classification[i].value;
        }
        memmove(features_bp_form, features_bp_form + READ_OFFSET_BP_FORM, BYTES_TO_KEEP_BP_FORM);
    }
}