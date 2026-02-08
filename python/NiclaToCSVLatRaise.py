# NiclaRepCapture.py
import math
import serial
import csv
import os
import re
import time

# --------- ANPASSEN ----------
COM_PORT   = "COM11"   # dein COM-Port
BAUD_RATE  = 115200
BASE_FOLDER = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\LatRaise"
# -----------------------------

# Erstelle Ordner falls nötig
os.makedirs(BASE_FOLDER, exist_ok=True)

# Hilfsfunktion: nächster Index (DataTest01.csv, 02, ...)
def next_index(folder):
    files = os.listdir(folder)
    pattern = re.compile(r"LatRaiseSample(\d+)\.csv$")
    nums = [int(pattern.match(f).group(1)) for f in files if pattern.match(f)]
    return max(nums)+1 if nums else 1

# Spaltennamen (erste Zeile)
HEADER = [
    "accX_L","accY_L","accZ_L","gyroX_L","gyroY_L","gyroZ_L",
    "accX_R","accY_R","accZ_R","gyroX_R","gyroY_R","gyroZ_R",
    "accX_L_diff","accY_L_diff","accZ_L_diff", "accX_R_diff","accY_R_diff","accZ_R_diff",
    "accNorm_L","accNorm_R","diff_X","diff_Y","diff_Z","ms_since_start",
    "label"
]

# Öffne seriellen Port
ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
time.sleep(2)  # Warte Arduino boot

print("Warten auf Daten...")
state = "WAIT"  # WAIT oder RECORD
current_file = None
writer = None
f_handle = None
start_time = None  # Initialize start_time

# Variablen für Recording
label = "correct form"
# label = "shrugging shoulders"
# label = "arms too bent"
# label = "arms too straight"
# label = "incomplete motion"
# label = "using momentum"
# label = "raised too high"
# label = "too fast"
# label = "thumbs too high"
total_y = None
threshold_total = 1200.0  
threshold_exceeded = None

# Zu berechnende Variablen
acc_norm_left = None
acc_norm_right = None
left_x_old = 0.0
left_y_old = 0.0
left_z_old = 0.0
right_x_old = 0.0
right_y_old = 0.0
right_z_old = 0.0
diff_x = None
diff_y = None
diff_z = None


try:
    while True:
        raw = ser.readline().decode("utf-8", errors="ignore").strip()
        if not raw:
            continue

        # entferne eckige Klammern falls vorhanden und splitte auf , oder ;
        clean = raw.replace("[", "").replace("]", "")
        parts = re.split(r'[;,]', clean)
        parts = [p.strip() for p in parts if p.strip() != ""]

        # nur Zeilen mit genau 12 Werten verarbeiten
        if len(parts) != 12:
            # optional: debug anzeigen
            # print("ignored:", raw)
            continue

        try:
            values = [float(x) for x in parts]
        except ValueError:
            # falls ein Nicht-Float vorkommt (Robustheit)
            continue

        # Indizes für y-Werte:
        left_x  = values[0]   # accX left
        left_y  = values[1]   # accY left
        left_z  = values[2]   # accZ left
        right_x = values[6]   # accX right
        right_y = values[7]   # accY right
        right_z = values[8]   # accZ right
        
        left_x_diff = left_x - left_x_old
        left_y_diff = left_y - left_y_old
        left_z_diff = left_z - left_z_old
        right_x_diff = right_x - right_x_old
        right_y_diff = right_y - right_y_old
        right_z_diff = right_z - right_z_old
        
        left_x_old = left_x
        left_y_old = left_y
        left_z_old = left_z
        right_x_old = right_x
        right_y_old = right_y
        right_z_old = right_z

        if state == "WAIT":
            # Startbedingung
            if abs(left_y_diff) < 150 and abs(right_y_diff) < 150:
                # Beginne eine neue Wiederholung
                idx = next_index(BASE_FOLDER)
                fname = os.path.join(BASE_FOLDER, f"LatRaiseSample{idx:02d}.csv")
                f_handle = open(fname, "w", newline='')
                # Verwende Semikolon als Delimiter (Excel DE-kompatibel)
                writer = csv.writer(f_handle, delimiter=';')

                # schreibe Header (erste Zeile)
                writer.writerow(HEADER)
                
                f_handle.flush()
                os.fsync(f_handle.fileno())

                state = "RECORD"
                start_time = time.time()
                total_y = 0.0
                threshold_exceeded = False
                print(f"REP START -> Datei: {fname}")
                continue

        elif state == "RECORD":
            # Berechne zusätzliche Werte
            acc_norm_left = math.sqrt(left_x**2 + left_y**2 + left_z**2)
            acc_norm_right = math.sqrt(right_x**2 + right_y**2 + right_z**2)
            diff_x = right_x - left_x
            diff_y = right_y - left_y
            diff_z = right_z - left_z
            ms_since_start = int((time.time() - start_time)*1000)
            
            # schreibe Zeile in aktuelle Datei
            writer.writerow(
                values + [
                    left_x_diff, left_y_diff, left_z_diff,
                    right_x_diff, right_y_diff, right_z_diff,
                    acc_norm_left, acc_norm_right,
                    diff_x, diff_y, diff_z,
                    ms_since_start, label
                ]
            )
            f_handle.flush(); os.fsync(f_handle.fileno())

            # Status für Überschreiten prüfen (Erkennung des Umkehrpunkts)
            current_y_sum = left_y_diff + right_y_diff
            if (current_y_sum < -4000):
                threshold_exceeded = True

            # Ende-Bedingung prüfen (Rückkehr zum stabilen Zustand)
            if threshold_exceeded:
                # Prüfe, ob die AKTUELLE Beschleunigung wieder gering ist
                if (abs(current_y_sum) < 300): # <-- NEUE END-BEDINGUNG
                    # Ende der Wiederholung
                    f_handle.close()
                    print("REP END -> Datei abgeschlossen.")
                    current_file = None
                    writer = None
                    f_handle = None
                    state = "WAIT"
                    # kurze Pause, damit Startbedingung nicht sofort wieder greift
                    continue

        # kleine Pause, damit CPU nicht 100% läuft (optional)
        time.sleep(0.001)

except KeyboardInterrupt:
    print("Beendet durch Benutzer.")
finally:
    if f_handle and not f_handle.closed:
        f_handle.close()
    ser.close()