# Práctica de Laboratorio: API REST con ESP32

Práctica del **Tema 4.3 — Consumo de APIs y plataformas en la nube para IoT**, de la
asignatura **Internet de las Cosas** (Especialidad, ITIC – ITSOEH).

En esta práctica el alumno **genera su propia API REST** en la ESP32: la placa lee
sensores reales (DHT22 y LDR), publica los datos en JSON y cualquier cliente
(navegador, curl o Python) puede consumirlos. Primero se genera la API para
entender cómo se consume una.

El documento completo de la práctica (16 páginas, con marco teórico, diagramas,
pruebas con evidencia, problemas frecuentes, preguntas de análisis, retos extra y
rúbrica de 100 puntos) está en `practica-api-rest-esp32.pdf`.

## Archivos

| Archivo | Descripción |
| --- | --- |
| `API_REST_ESP32.ino` | Sketch de Arduino: la ESP32 como servidor de API REST (WebServer + ArduinoJson 7.x) |
| `cliente_api.py` | Cliente en Python (`requests`): lee sensores, enciende el LED y monitorea 5 lecturas |
| `ejemplos_curl.txt` | Los mismos consumos de la API hechos con curl |
| `practica-api-rest-esp32.pdf` | Documento completo de la práctica |

## Materiales

- Placa ESP32 (o Heltec V3)
- Sensor DHT22 de temperatura y humedad (se puede cambiar a DHT11 en una sola línea)
- Fotorresistencia LDR con divisor de tensión
- LED + resistencia de 220 Ω
- Protoboard y cables Dupont
- Arduino IDE con las librerías **DHT sensor library** (Adafruit) y **ArduinoJson 7.x**
- Computadora con Python 3 y la librería `requests` (`pip install requests`)

## Conexiones

```
DHT22            ESP32
  VCC    ------   3V3
  GND    ------   GND
  DATA   ------   GPIO 15

LDR              ESP32
  un extremo --- 3V3
  otro extremo - GPIO 34 y resistencia de 10 kΩ a GND

LED              ESP32
  ánodo  ------  GPIO 2 (con resistencia de 220 Ω)
  cátodo ------  GND
```

## Endpoints de la API

| Método | Ruta | Respuesta |
| --- | --- | --- |
| GET | `/api` | Índice de la API con la lista de endpoints |
| GET | `/api/estado` | Estado del nodo: IP, RSSI, memoria libre, uptime |
| GET | `/api/sensores` | Última lectura: temperatura, humedad, luz, LED y uptime |
| GET | `/api/sensores/temperatura` | Solo la temperatura en °C |
| GET | `/api/historial` | Últimas 10 lecturas (una cada 10 s) |
| POST | `/api/led` | Enciende o apaga el LED con `{"led": true}` |

Códigos de estado usados: **200** OK, **400** JSON inválido o campo faltante,
**404** ruta no encontrada y **503** sensor no disponible.

## Flujo de la información

```
  DHT22 + LDR
      |
      v
  ESP32 (lee sensores cada 10 s)
      |
      |  HTTP + JSON (puerto 80)
      v
  API REST en la ESP32
      |
      +---> Navegador
      +---> curl
      +---> cliente_api.py (Python + requests)
```

## Ejemplo de respuesta

```json
{
  "dispositivo": "esp32-mesa-03",
  "temperatura_c": 26.4,
  "humedad_pct": 58.0,
  "luz_pct": 72,
  "led": false,
  "uptime_s": 145
}
```

## Uso rápido

1. En `API_REST_ESP32.ino` escribe tu SSID y contraseña Wi-Fi y el nombre de tu
   nodo (`NOMBRE_NODO`).
2. Instala las librerías **DHT sensor library** y **ArduinoJson** en Arduino IDE.
3. Carga el sketch en la ESP32 y anota la IP que muestra el Monitor Serie.
4. Abre `http://<IP>/api/sensores` en el navegador.
5. Prueba los comandos de `ejemplos_curl.txt` (cambia la IP por la de tu placa).
6. Corre el cliente Python: `python cliente_api.py` (cambia `BASE` por tu IP).

## Evaluación

La práctica se evalúa sobre 100 puntos según la rúbrica del documento PDF:
funcionamiento de la API (endpoints y códigos de estado), evidencia de las 6
pruebas, cliente Python, preguntas de análisis y retos extra.
