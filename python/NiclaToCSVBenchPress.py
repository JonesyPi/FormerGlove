import math
import serial
import csv
import os
import re
import time
import msvcrt

# --------- ANPASSEN ----------
COM_PORT   = "COM3"
BAUD_RATE  = 115200
BASE_FOLDER = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPress"
# -----------------------------

os.makedirs(BASE_FOLDER, exist_ok=True)

def next_index(folder):
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
weight = 20

HEADER = [
    "accX_Right","accY_Right","accZ_Right","gyroX_Right","gyroY_Right","gyroZ_Right",
    "accX_Left","accY_Left","accZ_Left","gyroX_Left","gyroY_Left","gyroZ_Left",
    "accX_Left_diff","accY_Left_diff","accZ_Left_diff",
    "accX_Right_diff","accY_Right_diff","accZ_Right_diff",
    "accNorm_Left","accNorm_Right",
    "diff_X","diff_Y","diff_Z",
    "weight","ms_since_start","label"
]

# --------- SERIELL ----------
ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
time.sleep(2)

print("Bereit.")
print("ENTER = Aufnahme START / STOP")
print("L = Label wechseln | W = Gewicht ändern")
print("CTRL+C = Beenden\n")

# --------- STATE ----------
f_handle = None
writer = None
start_time = None
recording = False

left_x_old = left_y_old = left_z_old = 0.0
right_x_old = right_y_old = right_z_old = 0.0

# --------- FUNKTIONEN ----------
def start_recording():
    global f_handle, writer, start_time, recording

    idx = next_index(BASE_FOLDER)
    fname = os.path.join(BASE_FOLDER, f"BenchPressSample{idx:02d}.csv")

    f_handle = open(fname, "w", newline="")
    writer = csv.writer(f_handle, delimiter=";")
    writer.writerow(HEADER)

    start_time = time.time()
    recording = True

    print(f"\nAUFNAHME START -> {fname}")
    print(f"Label: {current_label} | Gewicht: {weight} kg")

def stop_recording():
    global f_handle, writer, recording
    if f_handle:
        f_handle.close()
    f_handle = None
    writer = None
    recording = False
    print("AUFNAHME STOP\n")

def switch_label():
    global label_index, current_label
    label_index = (label_index + 1) % len(LABELS)
    current_label = LABELS[label_index]
    print(f"Neues Label: [{label_index}] {current_label}")

def change_weight():
    global weight
    try:
        w = input("Neues Gewicht (kg): ")
        weight = float(w)
        print(f"Gewicht gesetzt auf: {weight} kg")
    except ValueError:
        print("Ungültige Eingabe – Gewicht nicht geändert.")

# --------- MAIN LOOP ----------
try:
    while True:

        # --------- TASTEN ----------
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

        raw = ser.readline().decode("utf-8", errors="ignore").strip()
        if not raw or not recording:
            continue

        clean = raw.replace("[", "").replace("]", "")
        parts = re.split(r"[;,]", clean)
        parts = [p.strip() for p in parts if p.strip()]

        if len(parts) != 12:
            continue

        try:
            values = [float(x) for x in parts]
        except ValueError:
            continue

        left_x, left_y, left_z = values[0], values[1], values[2]
        right_x, right_y, right_z = values[6], values[7], values[8]

        left_x_diff = left_x - left_x_old
        left_y_diff = left_y - left_y_old
        left_z_diff = left_z - left_z_old
        right_x_diff = right_x - right_x_old
        right_y_diff = right_y - right_y_old
        right_z_diff = right_z - right_z_old

        left_x_old, left_y_old, left_z_old = left_x, left_y, left_z
        right_x_old, right_y_old, right_z_old = right_x, right_y, right_z

        acc_norm_left = math.sqrt(left_x**2 + left_y**2 + left_z**2)
        acc_norm_right = math.sqrt(right_x**2 + right_y**2 + right_z**2)

        diff_x = right_x - left_x
        diff_y = right_y - left_y
        diff_z = right_z - left_z

        ms_since_start = int((time.time() - start_time) * 1000)

        writer.writerow(
            values + [
                left_x_diff, left_y_diff, left_z_diff,
                right_x_diff, right_y_diff, right_z_diff,
                acc_norm_left, acc_norm_right,
                diff_x, diff_y, diff_z,
                weight, ms_since_start, current_label
            ]
        )

        time.sleep(0.001)

except KeyboardInterrupt:
    print("\nProgramm beendet.")

finally:
    if f_handle:
        f_handle.close()
    ser.close()
