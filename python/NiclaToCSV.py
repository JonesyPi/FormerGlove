import serial
import csv
import os

ser = serial.Serial("COM9", 9600)

output_file = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\NiclaData.csv"

print("Speichere in:", output_file)
print("Warten auf Daten...")

with open(output_file, "a", newline='') as f:
    writer = csv.writer(f, delimiter=';')
    
    writer.writerow(["accX_L","accY_L","accZ_L","gyroX_L","gyroY_L","gyroZ_L",
                     "accX_R","accY_R","accZ_R","gyroX_R","gyroY_R","gyroZ_R"])

    while True:
        line = ser.readline().decode("utf-8").strip()

        parts = line.split(',')
        if len(parts) == 12:
            try:
                values = [float(x) for x in parts]
                writer.writerow(values)
                f.flush()               # WICHTIG
                os.fsync(f.fileno())    # WICHTIG
                print(values)
            except ValueError:
                pass
