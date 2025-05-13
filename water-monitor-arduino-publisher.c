#include <WiFiS3.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"

// Configuración WiFi
const char* ssid = "LeninNo1Fan";
const char* password = "MikeTutos";
const char* serverHost = "water-monitor-ts.onrender.com"; 
const int serverPort = 80; // Puerto HTTP estándar
const char* websocketPath = "/"; // Ruta WebSocket

// Configuración del sensor
#define CONDUCT_PIN A1
float conductividad_cal_slope = 3.0;
float conductividad_cal_offset = 0.0;

// Matriz LED
ArduinoLEDMatrix matrix;
byte frame[8][12] = {0};

// Control de tiempo
unsigned long lastSensorUpdate = 0;
const long sensorInterval = 3000; // 3 segundos entre lecturas
unsigned long lastReconnect = 0;
const long reconnectInterval = 10000; // 10 segundos entre reconexiones

// Estado de la conexión
WiFiClient client;
bool wsConnected = false;
bool wsRegistered = false;

void setup() {
  matrix.begin();
  Serial.begin(9600);
  pinMode(CONDUCT_PIN, INPUT);
  
  connectToWiFi();
  connectWebSocket();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Reconectar WiFi si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    showMatrixMessage("W");
    connectToWiFi();
  }
  
  // Reconectar WebSocket si es necesario
  if (!wsConnected && currentMillis - lastReconnect > reconnectInterval) {
    showMatrixMessage("S");
    connectWebSocket();
  }

  // Leer y enviar datos del sensor
  if (wsConnected && wsRegistered && currentMillis - lastSensorUpdate >= sensorInterval) {
    float conductivity = readConductivity();
    showValueOnMatrix(conductivity);
    sendSensorData(conductivity);
    lastSensorUpdate = currentMillis;
  }
  
  // Procesar respuestas del servidor
  if (wsConnected && client.available()) {
    processWebSocketData();
  }
}

void connectToWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
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
    showMatrixMessage("C");
  } else {
    Serial.println("\nFalló la conexión WiFi");
    showMatrixMessage("F");
  }
}

void connectWebSocket() {
  Serial.println("Conectando a WebSocket server...");
  
  if (client.connect(serverHost, serverPort)) {
    Serial.println("TCP conectado! Iniciando handshake WebSocket...");
    
    // Handshake WebSocket
    client.println("GET " + String(websocketPath) + " HTTP/1.1");
    client.println("Host: " + String(serverHost));
    client.println("Upgrade: websocket");
    client.println("Connection: Upgrade");
    client.println("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==");
    client.println("Sec-WebSocket-Version: 13");
    client.println();
    
    // Esperar respuesta con timeout
    unsigned long timeout = millis() + 5000;
    while (client.available() == 0 && millis() < timeout) {
      delay(10);
    }
    
    if (client.available()) {
      String response = client.readStringUntil('\n');
      Serial.println("Respuesta: " + response);
      
      if (response.indexOf("HTTP/1.1 101") >= 0) {
        Serial.println("¡Handshake WebSocket exitoso!");
        
        // Limpiar buffer
        while (client.available()) {
          client.read();
        }
        
        wsConnected = true;
        registerAsPublisher();
        showMatrixMessage("OK");
      } else {
        Serial.println("Error en handshake WebSocket");
        client.stop();
        wsConnected = false;
      }
    } else {
      Serial.println("No hubo respuesta del servidor");
      client.stop();
      wsConnected = false;
    }
  } else {
    Serial.println("No se pudo conectar al servidor");
    wsConnected = false;
  }
  
  lastReconnect = millis();
}

void registerAsPublisher() {
  Serial.println("Registrando como publisher...");
  
  // Crear mensaje JSON para registro
  StaticJsonDocument<128> doc;
  doc["type"] = "register";
  doc["role"] = "publisher";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Enviar mensaje WebSocket
  sendWebSocketMessage(jsonString);
}

float readConductivity() {
  int raw = analogRead(CONDUCT_PIN);
  float conductivity = raw * conductividad_cal_slope + conductividad_cal_offset;
  conductivity = constrain(conductivity, 0, 1500);
  
  Serial.print("Conductividad: ");
  Serial.print(conductivity);
  Serial.println(" µS/cm");
  
  return conductivity;
}

void sendSensorData(float conductivity) {
  // Crear JSON para datos del sensor
  StaticJsonDocument<128> doc;
  doc["C"] = conductivity;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Enviar mensaje WebSocket
  sendWebSocketMessage(jsonString);
}

void sendWebSocketMessage(String message) {
  // Enviar frame WebSocket
  client.write((uint8_t)0x81); // Texto, fin=1
  
  // Longitud
  if (message.length() < 126) {
  client.write((uint8_t)message.length());
} else {
  client.write((uint8_t)126);
  client.write((uint8_t)((message.length() >> 8) & 0xFF));
  client.write((uint8_t)(message.length() & 0xFF));
}
  
  // Mensaje
  client.print(message);
  
  Serial.println("Mensaje enviado: " + message);
}

void processWebSocketData() {
  // Leer encabezado WebSocket
  byte header = client.read();
  
  // Verificar si es un ping (0x89)
  if ((header & 0x0F) == 0x09) {
    Serial.println("Recibido PING - respondiendo con PONG");
    client.write((uint8_t)0x8A); // PONG
    client.write((uint8_t)0x00); // Longitud cero
    return;
  }
  
  // Leer longitud
  byte lengthByte = client.read();
  int length = lengthByte & 0x7F;
  
  // Leer el mensaje
  String message = "";
  for (int i = 0; i < length; i++) {
    if (client.available()) {
      message += (char)client.read();
    }
  }
  
  Serial.println("Mensaje recibido: " + message);
  
  // Procesar el mensaje
  if (message == "ping") {
    sendWebSocketMessage("pong");
  } else if (message == "connected") {
    Serial.println("Conexión WebSocket confirmada");
  } else {
    // Intentar parsear como JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
      // Manejar respuesta de registro
      if (doc["type"] == "registered" && doc["role"] == "publisher") {
        wsRegistered = true;
        Serial.println("Registro como publisher confirmado");
      }
      // Manejar desconexión
      else if (doc["type"] == "disconnect") {
        Serial.println("Servidor solicitó desconexión");
        wsConnected = false;
        wsRegistered = false;
      }
    }
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