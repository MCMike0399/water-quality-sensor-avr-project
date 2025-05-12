#include <WiFiS3.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"

// Configuración WiFi
const char* ssid = "TU_SSID_WIFI";
const char* password = "TU_PASSWORD_WIFI";
const char* serverIP = "192.168.1.X"; // IP del servidor Python
const int serverPort = 8081;
const char* websocketPath = "/ws";

// Definición del pin para el sensor de conductividad
#define CONDUCT_PIN A1

// Parámetros de calibración
float conductividad_cal_slope = 3.0;
float conductividad_cal_offset = 0.0;

// Matriz LED
ArduinoLEDMatrix matrix;
byte frame[8][12] = {0};

// Control de tiempo
unsigned long lastSensorUpdate = 0;
const long sensorInterval = 3000; // 3 segundos entre lecturas
unsigned long lastWsReconnect = 0;
const long reconnectInterval = 10000; // 10 segundos entre intentos de reconexión

// Estado de la conexión
WiFiClient client;
bool wsConnected = false;

void setup() {
  // Inicializar matriz LED
  matrix.begin();
  
  // Inicializar comunicación serial
  Serial.begin(9600);
  while (!Serial && millis() < 3000);
  
  // Configurar pin del sensor
  pinMode(CONDUCT_PIN, INPUT);
  
  // Conectar a WiFi
  connectToWiFi();
  
  // Conectar al WebSocket
  connectWebSocket();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Reconectar WiFi si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    showMatrixMessage("W"); // Mensaje "W" para WiFi
    connectToWiFi();
  }
  
  // Intentar reconectar WebSocket periodicamente
  if (!wsConnected && currentMillis - lastWsReconnect > reconnectInterval) {
    showMatrixMessage("S"); // Mensaje "S" para Socket
    connectWebSocket();
  }

  // Leer sensor y enviar datos cada intervalo
  if (wsConnected && currentMillis - lastSensorUpdate >= sensorInterval) {
    // Leer sensor de conductividad
    float conductivity = readConductivity();
    
    // Mostrar valor en la matriz LED
    showValueOnMatrix(conductivity);
    
    // Enviar datos al servidor
    sendSensorData(conductivity);
    
    lastSensorUpdate = currentMillis;
  }
  
  // Procesar respuestas del servidor si hay
  if (wsConnected && client.available()) {
    processWebSocketData();
  }
}

void connectToWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Esperar conexión (con timeout)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    showMatrixMessage("C"); // Mensaje "C" para Conectado
  } else {
    Serial.println("\nFalló la conexión WiFi");
    showMatrixMessage("F"); // Mensaje "F" para Fallo
  }
}

void connectWebSocket() {
  if (client.connect(serverIP, serverPort)) {
    Serial.println("Conectado al servidor!");
    
    // Realizar handshake WebSocket
    client.println("GET " + String(websocketPath) + " HTTP/1.1");
    client.println("Host: " + String(serverIP) + ":" + String(serverPort));
    client.println("Upgrade: websocket");
    client.println("Connection: Upgrade");
    client.println("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==");
    client.println("Sec-WebSocket-Version: 13");
    client.println("X-Device-Type: arduino-publisher"); // Identificar como publisher
    client.println();
    
    // Esperar respuesta con timeout
    unsigned long timeout = millis() + 5000;
    while (client.available() == 0 && millis() < timeout) {
      delay(10);
    }
    
    if (client.available()) {
      String response = client.readStringUntil('\n');
      Serial.println("Respuesta: " + response);
      
      // Limpiar buffer
      while (client.available()) {
        client.read();
      }
      
      wsConnected = true;
      showMatrixMessage("OK");
    } else {
      Serial.println("No hubo respuesta del servidor");
      client.stop();
      wsConnected = false;
    }
  } else {
    Serial.println("No se pudo conectar al servidor");
    wsConnected = false;
  }
  
  lastWsReconnect = millis();
}

float readConductivity() {
  int raw = analogRead(CONDUCT_PIN);
  
  // Convertir a voltaje (0-5V)
  float voltage = raw * (5.0 / 1023.0);
  
  // Aplicar calibración
  float conductivity = raw * conductividad_cal_slope + conductividad_cal_offset;
  
  // Limitar a rango válido
  conductivity = constrain(conductivity, 0, 1500);
  
  Serial.print("Conductividad: ");
  Serial.print(conductivity);
  Serial.println(" µS/cm");
  
  return conductivity;
}

void sendSensorData(float conductivity) {
  // Crear JSON
  StaticJsonDocument<128> doc;
  doc["type"] = "data";
  doc["C"] = conductivity;
  
  // Serializar
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Enviar frame WebSocket
  client.write(0x81); // Texto, fin=1
  
  // Longitud
  if (jsonString.length() < 126) {
    client.write(jsonString.length());
  } else {
    client.write(126);
    client.write((jsonString.length() >> 8) & 0xFF);
    client.write(jsonString.length() & 0xFF);
  }
  
  // Datos sin máscara (los servidores no necesitan enmascarar)
  client.print(jsonString);
  
  Serial.print("Datos enviados: ");
  Serial.println(jsonString);
}

void processWebSocketData() {
  // Implementación básica - solo consume datos
  while (client.available()) {
    client.read();
  }
}

void showMatrixMessage(String message) {
  // Mostrar un mensaje simple en la matriz
  memset(frame, 0, sizeof(frame));
  
  // Implementación simple para mostrar letras
  if (message == "W") {
    // Mostrar W - patrón simple
    frame[3][3] = 1; frame[3][8] = 1;
    frame[4][4] = 1; frame[4][6] = 1; frame[4][7] = 1;
    frame[5][5] = 1;
  } else if (message == "S") {
    // Mostrar S - patrón simple
    frame[3][4] = 1; frame[3][5] = 1; frame[3][6] = 1; frame[3][7] = 1;
    frame[4][4] = 1;
    frame[5][4] = 1; frame[5][5] = 1; frame[5][6] = 1; frame[5][7] = 1;
    frame[6][7] = 1;
    frame[7][4] = 1; frame[7][5] = 1; frame[7][6] = 1; frame[7][7] = 1;
  } else if (message == "F") {
    // Mostrar F
    frame[3][4] = 1; frame[3][5] = 1; frame[3][6] = 1; frame[3][7] = 1;
    frame[4][4] = 1;
    frame[5][4] = 1; frame[5][5] = 1; frame[5][6] = 1;
    frame[6][4] = 1;
    frame[7][4] = 1;
  } else if (message == "C") {
    // Mostrar C
    frame[3][4] = 1; frame[3][5] = 1; frame[3][6] = 1; frame[3][7] = 1;
    frame[4][4] = 1;
    frame[5][4] = 1;
    frame[6][4] = 1;
    frame[7][4] = 1; frame[7][5] = 1; frame[7][6] = 1; frame[7][7] = 1;
  } else if (message == "OK") {
    // Mostrar un check
    frame[3][6] = 1;
    frame[4][7] = 1;
    frame[5][8] = 1;
    frame[6][7] = 1;
    frame[7][6] = 1;
    frame[8][5] = 1;
    frame[9][4] = 1;
  }
  
  matrix.renderBitmap(frame, 8, 12);
  delay(1000);
}

void showValueOnMatrix(float value) {
  // Mostrar el valor en formato numérico
  // Implementación simplificada - solo muestra un patrón según el valor
  memset(frame, 0, sizeof(frame));
  
  // Crear un patrón que varía con el valor
  int intensity = map(constrain(value, 0, 1500), 0, 1500, 1, 8);
  
  for (int i = 0; i < intensity; i++) {
    frame[7-i][5] = 1;
    frame[7-i][6] = 1;
  }
  
  matrix.renderBitmap(frame, 8, 12);
}