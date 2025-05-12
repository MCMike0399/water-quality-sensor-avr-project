#include "WiFiS3.h"
#include "ArduinoJson.h"
#include "Arduino_LED_Matrix.h"

// Configuración WiFi
const char* ssid = "TU_SSID_WIFI";
const char* password = "TU_PASSWORD_WIFI";
const char* websocketHost = "192.168.1.X"; // Dirección IP del servidor Python
const int websocketPort = 8081;
const char* websocketPath = "/ws";

// Configuración del sensor de conductividad
#define CONDUCT_PIN A1
float conductividad_cal_slope = 3.0;
float conductividad_cal_offset = 0.0;

// Matriz LED
ArduinoLEDMatrix matrix;
byte frame[8][12] = {0};

// Patrones para animación de onda
byte wave[6][8][12] = {
  // Onda 1 (borde exterior)
  {
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1}
  },
  // Onda 2
  {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  // Onda 3
  {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,1,0,0,0,0,0,0,1,0,0},
    {0,0,1,0,0,0,0,0,0,1,0,0},
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  // Onda 4
  {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  // Onda 5
  {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  // Onda 6 (vacío)
  {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  }
};

// Patrones para dígitos 0-9 y punto decimal
byte digits[11][8][12] = {
  // Los mismos patrones de dígitos que en el código original
  // Solo incluyo el primer patrón para ahorrar espacio
  { // 0
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0},
    {0,1,1,0,0,0,0,0,1,1,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  // Los patrones para los dígitos 1-9 y el punto decimal irían aquí
  // Se omiten para brevedad
};

// Variables de control
unsigned long lastSensorUpdate = 0;
unsigned long lastFrameChange = 0;
unsigned long lastWebSocketReconnect = 0;
const long sensorInterval = 3000;        // 3 segundos entre lecturas
int frameDuration = 100;                 // Velocidad base de animación
int currentWaveFrame = 0;
float currentConductivity = 0.0;
bool showingValue = false;
bool connected = false;

// Cliente WiFi
WiFiClient client;
bool wsConnected = false;

void setup() {
  // Inicializar matriz LED
  matrix.begin();
  
  // Inicializar comunicación serial para depuración
  Serial.begin(9600);
  while (!Serial && millis() < 5000);
  
  // Configurar pin de sensor
  pinMode(CONDUCT_PIN, INPUT);
  
  // Conectar a WiFi
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
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalló la conexión WiFi!");
  }
  
  // Intentar conectar al WebSocket
  connectWebSocket();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Reconectar WiFi si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconectando a WiFi...");
    WiFi.begin(ssid, password);
    delay(5000);
  }
  
  // Reconectar WebSocket si es necesario (cada 10 segundos si no está conectado)
  if (!wsConnected && currentMillis - lastWebSocketReconnect > 10000) {
    connectWebSocket();
    lastWebSocketReconnect = currentMillis;
  }
  
  // Animar matriz LED si no está mostrando un valor
  if (!showingValue) {
    if (currentMillis - lastFrameChange >= frameDuration) {
      updateLoader();
      lastFrameChange = currentMillis;
    }
  }
  
  // Leer sensor y enviar datos cada intervalo
  if (currentMillis - lastSensorUpdate >= sensorInterval) {
    float conductivity = readConductivity();
    currentConductivity = conductivity;
    
    // Ajustar velocidad de animación según conductividad
    frameDuration = map(constrain(conductivity, 0, 1500), 0, 1500, 200, 50);
    
    // Mostrar valor en matriz LED
    showConductivityAnimation(conductivity);
    
    // Enviar datos al servidor WebSocket
    if (wsConnected) {
      sendConductivityData(conductivity);
    }
    
    lastSensorUpdate = currentMillis;
  }
  
  // Procesar datos WebSocket entrantes si hay
  if (wsConnected && client.available()) {
    String data = client.readStringUntil('\n');
    Serial.print("Recibido del servidor: ");
    Serial.println(data);
  }
}

void connectWebSocket() {
  if (client.connect(websocketHost, websocketPort)) {
    Serial.println("Conectado al servidor WebSocket!");
    
    // Realizar handshake WebSocket
    client.println("GET " + String(websocketPath) + " HTTP/1.1");
    client.println("Host: " + String(websocketHost) + ":" + String(websocketPort));
    client.println("Upgrade: websocket");
    client.println("Connection: Upgrade");
    client.println("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ=="); // Clave fija para simplificar
    client.println("Sec-WebSocket-Version: 13");
    client.println();
    
    // Esperar respuesta del servidor
    unsigned long timeout = millis() + 5000;
    while (client.available() == 0 && millis() < timeout) {
      delay(10);
    }
    
    if (client.available()) {
      // Leer respuesta del handshake (no la procesamos completamente para simplificar)
      String response = client.readStringUntil('\n');
      Serial.println("Respuesta inicial: " + response);
      
      // Limpiar buffer
      while (client.available()) {
        client.read();
      }
      
      wsConnected = true;
    } else {
      Serial.println("Falló el handshake WebSocket!");
      client.stop();
      wsConnected = false;
    }
  } else {
    Serial.println("No se pudo conectar al servidor WebSocket!");
    wsConnected = false;
  }
  
  lastWebSocketReconnect = millis();
}

// Leer sensor de conductividad y convertir a unidades físicas
float readConductivity() {
  int raw = analogRead(CONDUCT_PIN);
  float conductivity = raw * conductividad_cal_slope + conductividad_cal_offset;
  return constrain(conductivity, 0, 1500);
}

// Enviar datos de conductividad por WebSocket en formato JSON
void sendConductivityData(float conductivity) {
  // Crear objeto JSON
  StaticJsonDocument<128> doc;
  doc["C"] = conductivity;
  
  // Serializar a string
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Datos WebSocket deben ser enmascarados según el protocolo
  // Este es un enfoque simplificado
  client.write(0x81); // Texto, FIN=1
  
  // Longitud
  if (jsonString.length() < 126) {
    client.write(0x80 | jsonString.length()); // Con máscara, longitud < 126
  } else {
    client.write(0x80 | 126); // Con máscara, longitud = 126
    client.write((jsonString.length() >> 8) & 0xFF);
    client.write(jsonString.length() & 0xFF);
  }
  
  // Máscara (fija para simplificar)
  byte mask[4] = {0x12, 0x34, 0x56, 0x78};
  client.write(mask, 4);
  
  // Datos enmascarados
  for (size_t i = 0; i < jsonString.length(); i++) {
    client.write(jsonString[i] ^ mask[i % 4]);
  }
  
  Serial.print("Datos enviados: ");
  Serial.println(jsonString);
}

// Actualizar animación de carga en matriz LED
void updateLoader() {
  memcpy(frame, wave[currentWaveFrame], sizeof(frame));
  matrix.renderBitmap(frame, 8, 12);
  currentWaveFrame = (currentWaveFrame + 1) % 6;
}

// Mostrar dígito en matriz LED
void displayDigit(int digit) {
  if (digit >= 0 && digit <= 10) {
    memcpy(frame, digits[digit], sizeof(frame));
    matrix.renderBitmap(frame, 8, 12);
  }
}

// Mostrar valor de conductividad como secuencia de dígitos
void showConductivityAnimation(float value) {
  showingValue = true;
  
  int intPart = (int)value;
  int decimals = (int)((value - intPart) * 100);
  
  int digit1 = intPart / 100;
  int digit2 = (intPart / 10) % 10;
  int digit3 = intPart % 10;
  int decimal1 = decimals / 10;
  int decimal2 = decimals % 10;
  
  clearFrame();
  
  if (digit1 > 0) {
    displayDigit(digit1);
    delay(200);
    clearFrame();
    matrix.renderBitmap(frame, 8, 12);
    delay(50);
  }
  
  displayDigit(digit2);
  delay(200);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  displayDigit(digit3);
  delay(200);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  displayDigit(10);  // Punto decimal
  delay(150);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  displayDigit(decimal1);
  delay(200);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  displayDigit(decimal2);
  delay(200);
  
  showingValue = false;
}

// Limpiar buffer de frame
void clearFrame() {
  memset(frame, 0, sizeof(frame));
}