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

## Tres retos (nivel medio-avanzado)

Estos retos se construyen **sobre la API que ya hiciste en la práctica**: mismos
sensores, mismos endpoints, mismo JSON. No se trata de empezar de cero, sino de
llevar esa API a un sistema real. Cada reto vale **10 puntos extra** sobre la
práctica (máximo 30).

> La IA te puede ayudar a escribir código, pero **la evidencia es física**: si no
> se ve el hardware respondiendo en el laboratorio, el reto no está completo.
> Además, cada entrega debe incluir un párrafo corto con **qué falló durante las
> pruebas y cómo lo resolviste** — eso es lo que demuestra que lo entendiste.

### Reto 1. Alarma automática con umbrales que sobreviven al reinicio

Tu API solo responde cuando alguien le pregunta. Haz que **la propia ESP32 vigile
los sensores y actúe sola**.

**Qué construir:**
- Nuevo endpoint `POST /api/umbrales` para configurar límites, por ejemplo
  `{"temp_max": 30, "humedad_min": 40, "luz_min": 20}`. Los umbrales deben
  guardarse en memoria no volátil (librería `Preferences`/NVS) y **seguir ahí
  después de reiniciar la placa**.
- Cuando una lectura cruce un umbral, la ESP32 activa sola un buzzer o un relé
  (un actuador físico, no solo el LED) y registra el evento.
- Nuevo endpoint `GET /api/eventos` con la lista de eventos (qué umbral, valor
  medido y `uptime_s` del momento).
- El actuador debe apagarse solo cuando la lectura vuelva a rango **con
  histéresis** (por ejemplo, la alarma se enciende a 30 °C y se apaga hasta bajar
  a 28 °C), para que no parpadee con el ruido del sensor.

**Evidencia obligatoria:** video corto donde calientas el DHT22 con la mano o
una secadora, suena el buzzer, y luego **reinicias la ESP32 y muestras que los
umbrales siguen configurados** (con un `GET` o por el Monitor Serie).

### Reto 2. Red de 3 nodos con tablero en Python que aguanta un nodo caído

Una API sola es un juguete. Conecta **3 ESP32 (3 mesas)** y haz un tablero que
las lea a todas sin caerse cuando una falle.

**Qué construir:**
- Protege tu API con un token: el cliente debe enviar el encabezado
  `X-API-Key: <tu clave>`; sin token o con token inválido la ESP32 responde
  **401**. Actualiza `cliente_api.py` y `ejemplos_curl.txt` con el token.
- Un tablero en Python que consulte los 3 nodos **en paralelo** (hilos o
  `asyncio`), muestre temperatura, humedad y luz de cada uno, y detecte cuál
  está fuera de línea (timeout de 3 s).
- Cuando un nodo no responda, el tablero **no se debe cerrar ni bloquear**:
  marca ese nodo como "sin conexión", conserva su última lectura marcada como
  desactualizada, sigue leyendo los otros dos, y lo reincorpora solo cuando
  vuelva.
- Guarda todas las lecturas en un archivo CSV con fecha, hora, nodo y valores.

**Evidencia obligatoria:** video o registro donde **desconectas un nodo a la
mitad del monitoreo** y el tablero lo marca fuera de línea sin detenerse, y un
`curl` sin token que devuelva 401.

### Reto 3. Telemetría a la nube con búfer offline (store-and-forward)

Tu API vive en la red local. Lleva los datos a una **plataforma gratuita en la
nube** (ThingSpeak, Blynk, Firebase o un broker MQTT) sin perder datos cuando
el Wi-Fi se caiga.

**Qué construir:**
- La ESP32 (o el cliente Python, a tu elección) reenvía una lectura a la nube
  cada 30 s.
- Cuando no haya Wi-Fi o la nube no responda, las lecturas se guardan en un
  **búfer circular local** (mínimo 50 lecturas) y **no se pierden**.
- Al recuperar la conexión, el búfer se reenvía **en orden y sin duplicados**:
  cada lectura lleva un identificador único o marca de tiempo para que la nube
  no la cuente dos veces.
- La nube debe mostrar la gráfica de temperatura y humedad del día.

**Evidencia obligatoria:** apaga el Wi-Fi (o el hotspot) por 2 minutos,
vuelve a encenderlo y muestra en el tablero de la nube que **el hueco se llenó
con los datos del búfer y que no hay lecturas duplicadas**.

## Evaluación

La práctica se evalúa sobre 100 puntos según la rúbrica del documento PDF:
funcionamiento de la API (endpoints y códigos de estado), evidencia de las 6
pruebas, cliente Python, preguntas de análisis y retos extra.
