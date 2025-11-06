import requests

# Dein API Key
API_KEY = "ei_61cd15b94446d9a2c41b9c89409ec74c3163268b5a498fe004ad52fd6e4a3333"

# Datei, die du hochladen willst (z. B. Sensorwerte)
file_path = "C:\\Users\\daned\\OneDrive\\Dokumente\\Diplomprojekt\\FormerGlove\\edge_impulse_forwarder\\test.csv"

# Metadaten (z. B. Label)
label = "test"

# API-Endpunkt
url = "https://ingestion.edgeimpulse.com/api/training/files"

# Datei und Header
headers = {
    "x-api-key": API_KEY
}

files = {
    "data": (file_path, open(file_path, "rb")),
    "label": (None, label),
}

# Anfrage senden
response = requests.post(url, headers=headers, files=files)

print(response.status_code, response.text)
