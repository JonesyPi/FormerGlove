# NiclaRepCapture.py
import math
import serial
import csv
import os
import re
import time

# --------- ANPASSEN ----------
COM_PORT   = "COM11"
BAUD_RATE  = 115200
BASE_FOLDER = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPress"
# -----------------------------

os.makedirs(BASE_FOLDER, exist_ok=True)

def next_index(folder):
    files = os.listdir(folder)
    pattern = re.compile(r"BenchPressSample(\d+)\.csv$")
    nums = [int(pattern.match(f).group(1)) for f in files if pattern.match(f)]
    return max(nums) + 1 if nums else 1

HEADER = [
    "accX_L","accY_L","accZ_L","gyroX_L","gyroY_L","gyroZ_L",
    "accX_R","accY_R","accZ_R","gyroX_R","gyroY_R","gyroZ_R",
    "accX_L_diff","accY_L_diff","accZ_L_diff",
    "accX_R_diff","accY_R_diff","accZ_R_diff",
    "accNorm_L","accNorm_R",
    "diff_X","diff_Y","diff_Z",
    "ms_since_start",
    "label"
]

# Label fix setzen
label = "correct form"

# Seriell öffnen
ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
time.sleep(2)

# Neue Datei EINMAL erstellen
idx = next_index(BASE_FOLDER)
fname = os.path.join(BASE_FOLDER, f"BenchPressSample{idx:02d}.csv")
f_handle = open(fname, "w", newline="")
writer = csv.writer(f_handle, delimiter=';')
writer.writerow(HEADER)
f_handle.flush()
os.fsync(f_handle.fileno())

print(f"Logging gestartet -> {fname}")

start_time = time.time()

# Alte Werte für Diffs
left_x_old = left_y_old = left_z_old = 0.0
right_x_old = right_y_old = right_z_old = 0.0

try:
    while True:
        raw = ser.readline().decode("utf-8", errors="ignore").strip()
        if not raw:
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
                ms_since_start,
                label
            ]
        )

        f_handle.flush()
        os.fsync(f_handle.fileno())

        time.sleep(0.001)

except KeyboardInterrupt:
    print("Beendet durch Benutzer.")

finally:
    if f_handle and not f_handle.closed:
        f_handle.close()
    ser.close()
