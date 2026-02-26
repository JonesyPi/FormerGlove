import math
import serial
import csv
import os
import re
import time
import msvcrt

# --------- ADJUST HERE ----------
COM_PORT   = "COM3"
BAUD_RATE  = 115200
BASE_FOLDER = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPress"
# --------------------------------

# Create folder if it does not exist
os.makedirs(BASE_FOLDER, exist_ok=True)

def next_index(folder):
    """
    Finds the next free file index (BenchPressSampleXX.csv)
    so that no existing file gets overwritten.
    """
    files = os.listdir(folder)
    pattern = re.compile(r"BenchPressSample(\d+)\.csv$")
    nums = [int(pattern.match(f).group(1)) for f in files if pattern.match(f)]
    return max(nums) + 1 if nums else 1

# --------- LABELS ----------
LABELS = [
    "correct form",
    "bent wrists",
    "angled elbows",
    "bar not touching chest",
    "incomplete motion",
    "using momentum",
    "incorrect grip width",
    "too fast"
]

label_index = 0
current_label = LABELS[label_index]
weight = 60  # default weight in kg

# CSV header (feature names)
HEADER = [
    "accX_Right","accY_Right","accZ_Right","gyroX_Right","gyroY_Right","gyroZ_Right",
    "accX_Left","accY_Left","accZ_Left","gyroX_Left","gyroY_Left","gyroZ_Left",
    "accX_Left_diff","accY_Left_diff","accZ_Left_diff",
    "accX_Right_diff","accY_Right_diff","accZ_Right_diff",
    "accNorm_Left","accNorm_Right",
    "diff_X","diff_Y","diff_Z",
    "weight","ms_since_start","label"
]

# --------- SERIAL ----------
# Open serial connection to microcontroller
ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # wait for board reset

print("Ready.")
print("ENTER = START / STOP recording")
print("L = switch label | W = change weight")
print("CTRL+C = Exit\n")

# --------- STATE ----------
f_handle = None
writer = None
start_time = None
recording = False

# Store previous acceleration values for delta calculation
left_x_old = left_y_old = left_z_old = 0.0
right_x_old = right_y_old = right_z_old = 0.0

# --------- FUNCTIONS ----------
def start_recording():
    global f_handle, writer, start_time, recording

    idx = next_index(BASE_FOLDER)
    fname = os.path.join(BASE_FOLDER, f"BenchPressSample{idx:02d}.csv")

    f_handle = open(fname, "w", newline="")
    writer = csv.writer(f_handle, delimiter=";")
    writer.writerow(HEADER)

    start_time = time.time()
    recording = True

    print(f"\nRECORDING START -> {fname}")
    print(f"Label: {current_label} | Weight: {weight} kg")

def stop_recording():
    global f_handle, writer, recording
    if f_handle:
        f_handle.close()
    f_handle = None
    writer = None
    recording = False
    print("RECORDING STOP\n")

def switch_label():
    """
    Cycles through the predefined labels.
    Only allowed when not recording.
    """
    global label_index, current_label
    label_index = (label_index + 1) % len(LABELS)
    current_label = LABELS[label_index]
    print(f"New label: [{label_index}] {current_label}")

def change_weight():
    """
    Allows manual weight input.
    Only allowed when not recording.
    """
    global weight
    try:
        w = input("New weight (kg): ")
        weight = float(w)
        print(f"Weight set to: {weight} kg")
    except ValueError:
        print("Invalid input – weight not changed.")

# --------- MAIN LOOP ----------
try:
    while True:

        # --------- KEYBOARD INPUT ----------
        # Non-blocking key detection (Windows only: msvcrt)
        if msvcrt.kbhit():
            key = msvcrt.getch().lower()

            if key == b'\r':  # ENTER
                if not recording:
                    start_recording()
                else:
                    stop_recording()
                time.sleep(0.4)

            elif key == b'l' and not recording:
                switch_label()
                time.sleep(0.3)

            elif key == b'w' and not recording:
                change_weight()
                time.sleep(0.3)

        # Read one line from serial
        raw = ser.readline().decode("utf-8", errors="ignore").strip()
        if not raw or not recording:
            continue

        # Clean formatting (remove brackets etc.)
        clean = raw.replace("[", "").replace("]", "")
        parts = re.split(r"[;,]", clean)
        parts = [p.strip() for p in parts if p.strip()]

        # Expect exactly 12 values (6 left + 6 right)
        if len(parts) != 12:
            continue

        try:
            values = [float(x) for x in parts]
        except ValueError:
            continue

        # Extract acceleration only (gyro not used for diffs here)
        left_x, left_y, left_z = values[0], values[1], values[2]
        right_x, right_y, right_z = values[6], values[7], values[8]

        # --------- DERIVATIVES (frame-to-frame differences) ----------
        left_x_diff = left_x - left_x_old
        left_y_diff = left_y - left_y_old
        left_z_diff = left_z - left_z_old
        right_x_diff = right_x - right_x_old
        right_y_diff = right_y - right_y_old
        right_z_diff = right_z - right_z_old

        # Update old values
        left_x_old, left_y_old, left_z_old = left_x, left_y, left_z
        right_x_old, right_y_old, right_z_old = right_x, right_y, right_z

        # --------- VECTOR MAGNITUDE ----------
        # sqrt(x² + y² + z²)
        acc_norm_left = math.sqrt(left_x**2 + left_y**2 + left_z**2)
        acc_norm_right = math.sqrt(right_x**2 + right_y**2 + right_z**2)

        # Difference between gloves
        diff_x = right_x - left_x
        diff_y = right_y - left_y
        diff_z = right_z - left_z

        # Time since recording start (ms)
        ms_since_start = int((time.time() - start_time) * 1000)

        # Write everything into CSV
        writer.writerow(
            values + [
                left_x_diff, left_y_diff, left_z_diff,
                right_x_diff, right_y_diff, right_z_diff,
                acc_norm_left, acc_norm_right,
                diff_x, diff_y, diff_z,
                weight, ms_since_start, current_label
            ]
        )

        time.sleep(0.001)  # small delay to reduce CPU usage

except KeyboardInterrupt:
    print("\nProgram terminated.")

finally:
    if f_handle:
        f_handle.close()
    ser.close()