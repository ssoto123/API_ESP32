# cliente_api.py - Consume la API de la ESP32
import requests, time

BASE = "http://192.168.1.50"  # <-- la IP de TU placa

# GET: leer sensores y procesar el JSON como diccionario
r = requests.get(BASE + "/api/sensores", timeout=5)
print("Codigo:", r.status_code)        # esperado: 200
datos = r.json()
print("Temperatura:", datos["temperatura_c"], "C")
print("Luz:", datos["luz_pct"], "%")

# POST: actuar sobre el LED enviando un diccionario,
# requests lo convierte a JSON automaticamente
r = requests.post(BASE + "/api/led", json={"led": True},
                  timeout=5)
print("LED:", r.json())

# Monitoreo: 5 lecturas, una cada 5 segundos
for i in range(5):
    d = requests.get(BASE + "/api/sensores", timeout=5).json()
    print(i, d["temperatura_c"], d["humedad_pct"], d["luz_pct"])
    time.sleep(5)
