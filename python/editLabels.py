import csv
import os

# --------- ANPASSEN ----------
INPUT_FOLDER  = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPress"
OUTPUT_FOLDER = r"C:\Users\daned\OneDrive\Dokumente\Diplomprojekt\FormerGlove\csv\BenchPressSimple"
# -----------------------------

os.makedirs(OUTPUT_FOLDER, exist_ok=True)

for filename in os.listdir(INPUT_FOLDER):
    if not filename.lower().endswith(".csv"):
        continue

    input_path = os.path.join(INPUT_FOLDER, filename)
    output_path = os.path.join(OUTPUT_FOLDER, filename)

    with open(input_path, "r", newline="", encoding="utf-8") as f_in:
        reader = list(csv.reader(f_in, delimiter=";"))

    if len(reader) < 2:
        print(f"Übersprungen: {filename}")
        continue

    header = reader[0]
    data_rows = reader[1:]

    # finde die Label-Spalte
    try:
        label_index = header.index("label")
    except ValueError:
        print(f"Keine 'label'-Spalte in {filename}")
        continue

    new_rows = []
    for row in data_rows:
        if row[label_index] != "correct form":
            row[label_index] = "incorrect form"
        new_rows.append(row)

    with open(output_path, "w", newline="", encoding="utf-8") as f_out:
        writer = csv.writer(f_out, delimiter=";")
        writer.writerow(header)
        writer.writerows(new_rows)

    print(f"Verarbeitet: {filename}")

print("\nFertig: Alle Labels angepasst.")