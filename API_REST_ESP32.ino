// ============================================================
// API REST en ESP32 - Practica Tema 4.3
// La ESP32 lee sensores reales y GENERA su propia API:
//   GET  /api, /api/estado, /api/sensores,
//        /api/sensores/temperatura, /api/historial
//   POST /api/led
// Transporte: HTTP sobre TCP, puerto 80. Formato: JSON.
// ============================================================

// --- Librerias ----------------------------------------------
#include <WiFi.h>        // Conexion Wi-Fi de la ESP32
#include <WebServer.h>   // Servidor HTTP incluido en el
                           // soporte ESP32: registra rutas
                           // y despacha peticiones
#include <DHT.h>         // Sensor DHT22 / DHT11 (Adafruit)
#include <ArduinoJson.h> // Construye y lee JSON SIN errores
                           // de sintaxis (seccion 3.2)

// --- Credenciales y nombre del nodo --------------------------
// Sustituye por los datos de la red del laboratorio.
// El nombre identifica a TU nodo en cada respuesta JSON:
// cambialo por tu mesa o tu equipo.
const char* ssid     = "TU_SSID_WIFI";
const char* password = "TU_PASSWORD_WIFI";
const char* NOMBRE_NODO = "esp32-mesa-03";

// --- Pines ----------------------------------------------------
#define PIN_DHT 15   // Datos del DHT22
#define PIN_LDR 34   // ADC del divisor del LDR (solo entrada)
#define PIN_LED 2    // LED que controla el POST /api/led
#define TIPO_DHT DHT22  // Cambia a DHT11 si es tu sensor

DHT dht(PIN_DHT, TIPO_DHT);  // Objeto del sensor
WebServer server(80);        // Servidor HTTP en el puerto 80,
                             // el puerto estandar de la web

// --- Estado compartido ---------------------------------------
// Ultima lectura valida: la usan /api/sensores y el historial.
float temperatura = 0, humedad = 0;
int   luzPct = 0;
bool  ledEncendido = false;

// --- Historial en memoria ------------------------------------
// Guardamos las ultimas 10 lecturas (una cada 10 segundos)
// para que /api/historial tenga algo que ofrecer sin usar
// una base de datos: la RAM es la "plataforma" de esta API.
struct Lectura { float t, h; int luz; unsigned long seg; };
Lectura historial[10];
int histTotal = 0;  // Cuantas posiciones validas hay (max. 10)

// ============================================================
// leerSensores(): lee el hardware UNA vez y actualiza el
// estado. Devuelve false si el DHT no respondio (muy comun
// si se le pregunta antes de 2 s desde la lectura anterior).
// ============================================================
bool leerSensores() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) return false;  // Lectura fallida
  temperatura = t;
  humedad = h;
  // El ADC entrega 0..4095. Lo convertimos a porcentaje
  // para que el JSON sea entendible sin conocer el hardware.
  luzPct = map(analogRead(PIN_LDR), 0, 4095, 0, 100);
  return true;
}

// ============================================================
// enviarJSON(): TODAS las respuestas de la API salen por
// aqui. Asi garantizamos dos cosas: el Content-Type siempre
// es application/json y el codigo de estado es explicito.
// ============================================================
void enviarJSON(int codigo, JsonDocument& doc) {
  String cuerpo;
  serializeJson(doc, cuerpo);  // Documento -> texto JSON valido
  server.send(codigo, "application/json", cuerpo);
}

// --- Handler de GET /api/sensores -----------------------------
// Un handler es a WebServer lo que un callback era a CoAP:
// la funcion que se ejecuta cuando llega esa ruta exacta.
void handleSensores() {
  Serial.println("GET /api/sensores");  // Evidencia en consola
  if (!leerSensores()) {
    // El sensor fallo: NO inventamos un dato. Respondemos
    // 503 con un JSON de error, como haria una API real.
    JsonDocument doc;
    doc["error"] = "sensor no disponible, intenta de nuevo";
    enviarJSON(503, doc);
    return;
  }
  JsonDocument doc;
  doc["dispositivo"]    = NOMBRE_NODO;
  doc["temperatura_c"]  = temperatura;
  doc["humedad_pct"]    = humedad;
  doc["luz_pct"]        = luzPct;
  doc["led"]            = ledEncendido;
  doc["uptime_s"]       = millis() / 1000;
  enviarJSON(200, doc);  // 200 OK + el JSON de arriba
}

// --- Handler de GET /api --------------------------------------
void handleIndice() {
  JsonDocument doc;
  doc["api"]     = "API REST de sensores - ESP32";
  doc["version"] = "1.0";
  doc["dispositivo"] = NOMBRE_NODO;
  JsonArray rutas = doc["endpoints"].to<JsonArray>();
  rutas.add("GET /api/estado");
  rutas.add("GET /api/sensores");
  rutas.add("GET /api/sensores/temperatura");
  rutas.add("GET /api/historial");
  rutas.add("POST /api/led  {\"led\":true}");
  enviarJSON(200, doc);
}

// --- Handler de GET /api/estado --------------------------------
void handleEstado() {
  JsonDocument doc;
  doc["dispositivo"]  = NOMBRE_NODO;
  doc["ip"]           = WiFi.localIP().toString();
  doc["rssi_dbm"]     = WiFi.RSSI();  // Potencia Wi-Fi recibida
  doc["heap_libre"]   = ESP.getFreeHeap();
  doc["uptime_s"]     = millis() / 1000;
  enviarJSON(200, doc);
}

// --- Handler de GET /api/sensores/temperatura -------------------
void handleTemperatura() {
  if (!leerSensores()) {
    JsonDocument doc; doc["error"] = "sensor no disponible";
    enviarJSON(503, doc); return;
  }
  JsonDocument doc;
  doc["temperatura_c"] = temperatura;
  enviarJSON(200, doc);
}

// --- Handler de GET /api/historial -------------------------------
void handleHistorial() {
  JsonDocument doc;
  JsonArray arr = doc["historial"].to<JsonArray>();
  for (int i = 0; i < histTotal; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["temperatura_c"] = historial[i].t;
    o["humedad_pct"]   = historial[i].h;
    o["luz_pct"]       = historial[i].luz;
    o["uptime_s"]      = historial[i].seg;
  }
  enviarJSON(200, doc);
}

// --- Handler de POST /api/led -------------------------------------
// El cliente ENVIA JSON y la API actua. Validar antes de
// actuar es la mitad del trabajo de una API seria.
void handleLed() {
  Serial.println("POST /api/led");
  if (!server.hasArg("plain")) {  // No llego cuerpo
    JsonDocument e; e["error"] = "se esperaba JSON {\"led\":true}";
    enviarJSON(400, e); return;
  }
  JsonDocument doc;
  // Si el cuerpo no es JSON valido, deserializeJson lo dice:
  if (deserializeJson(doc, server.arg("plain"))) {
    JsonDocument e; e["error"] = "JSON invalido";
    enviarJSON(400, e); return;
  }
  if (!doc["led"].is<bool>()) {  // Falta el campo o no es bool
    JsonDocument e; e["error"] = "falta el campo led (true/false)";
    enviarJSON(400, e); return;
  }
  ledEncendido = doc["led"];
  digitalWrite(PIN_LED, ledEncendido ? HIGH : LOW);
  JsonDocument r; r["led"] = ledEncendido;
  enviarJSON(200, r);
}

// --- Handler de rutas NO registradas: 404 en JSON -----------------
void handleNoEncontrada() {
  JsonDocument doc;
  doc["error"] = "ruta no encontrada";
  doc["ruta"]  = server.uri();
  enviarJSON(404, doc);
}

// ============================================================
// setup(): conectar, registrar rutas y arrancar. El registro
// de rutas ES la tabla de la seccion 4 convertida en codigo:
// si una ruta no se registra aqui, para el cliente no existe.
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConectado a la red");
  Serial.print("Tu API vive en: http://");
  Serial.println(WiFi.localIP());  // ANOTA esta IP

  // --- Tabla de rutas (metodo + ruta + handler) -------------
  server.on("/api", HTTP_GET, handleIndice);
  server.on("/api/estado", HTTP_GET, handleEstado);
  server.on("/api/sensores", HTTP_GET, handleSensores);
  server.on("/api/sensores/temperatura", HTTP_GET,
            handleTemperatura);
  server.on("/api/historial", HTTP_GET, handleHistorial);
  server.on("/api/led", HTTP_POST, handleLed);
  server.onNotFound(handleNoEncontrada);  // Todo lo demas: 404

  server.begin();
  Serial.println("Servidor HTTP iniciado (puerto 80)");
}

// ============================================================
// loop(): dos trabajos y nada mas.
//  1. handleClient() atiende peticiones: si se bloquea,
//     la API deja de responder aunque el Wi-Fi siga bien.
//  2. Cada 10 s guarda una lectura en el historial.
// millis() permite temporizar SIN delay(): handleClient()
// se sigue ejecutando mientras "esperamos".
// ============================================================
unsigned long ultimaLectura = 0;
void loop() {
  server.handleClient();
  if (millis() - ultimaLectura >= 10000) {
    ultimaLectura = millis();
    if (leerSensores()) {
      if (histTotal < 10) histTotal++;
      // Desplaza a la izquierda y deja la nueva al final:
      for (int i = 0; i < 9; i++) historial[i] = historial[i+1];
      historial[9] = { temperatura, humedad, luzPct,
                       millis() / 1000 };
    }
  }
}
