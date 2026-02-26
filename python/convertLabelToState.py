import csv
import os
import math

# --------- ANPASSEN ----------
INPUT_FOLDER  = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPress"
OUTPUT_FOLDER = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPressState"
# -----------------------------

os.makedirs(OUTPUT_FOLDER, exist_ok=True)

for filename in os.listdir(INPUT_FOLDER):
    if not filename.lower().endswith(".csv"):
        continue

    input_path = os.path.join(INPUT_FOLDER, filename)

    with open(input_path, "r", newline="", encoding="utf-8") as f_in:
        reader = list(csv.reader(f_in, delimiter=";"))

    if len(reader) < 2:
        print(f"Übersprungen (keine Daten): {filename}")
        continue

    header = reader[0]
    data_rows = reader[1:]
    
    # Split data into 60% running and 40% end
    total_rows = len(data_rows)
    split_index = math.floor(total_rows * 0.6) 
    
    running_rows = data_rows[:split_index]
    end_rows = data_rows[split_index:]

    # Remove .csv extension
    base_name = os.path.splitext(filename)[0]

    # Create output file paths
    running_path = os.path.join(OUTPUT_FOLDER, f"{base_name}_running.csv")
    end_path = os.path.join(OUTPUT_FOLDER, f"{base_name}_end.csv")

    # Write running file
    with open(running_path, "w", newline="", encoding="utf-8") as f_run:
        writer = csv.writer(f_run, delimiter=";")
        writer.writerow(header)
        writer.writerows(running_rows)

    # Write end file
    with open(end_path, "w", newline="", encoding="utf-8") as f_end:
        writer = csv.writer(f_end, delimiter=";")
        writer.writerow(header)
        writer.writerows(end_rows)

    print(f"Verarbeitet: {filename}")

print("\nAlle Dateien wurden erfolgreich aufgeteilt.")
