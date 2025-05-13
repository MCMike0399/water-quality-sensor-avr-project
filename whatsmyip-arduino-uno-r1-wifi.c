#include <WiFiS3.h>

char ssid[] = "LeninNo1Fan";
char pass[] = "MikeTutos";
const char ipServer[] = "api.ipify.org";

WiFiClient client;
int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 3000);
  Serial.println("Starting WiFi connection");
  
  // Print WiFi firmware version
  String fv = WiFi.firmwareVersion();
  Serial.print("Firmware version: ");
  Serial.println(fv);
  
  // Connect to WiFi with improved error handling
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  
  status = WiFi.begin(ssid, pass);
  
  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (status != WL_CONNECTED && millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
  }
  
  if (status != WL_CONNECTED) {
    Serial.print("Connection failed with status: ");
    Serial.println(status);
    return;
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Test connection to ipify
  getPublicIP();
}

void loop() {
  // Try again every 30 seconds if failed
  if (status == WL_CONNECTED) {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 30000) {
      getPublicIP();
      lastCheck = millis();
    }
  }
}

void getPublicIP() {
  Serial.println("\n------------------------------");
  Serial.print("Connecting to ");
  Serial.println(ipServer);
  
  if (client.connect(ipServer, 80)) {
    Serial.println("Connected to server");
    
    // Send HTTP request with explicit User-Agent
    client.println("GET / HTTP/1.1");
    client.println("Host: api.ipify.org");
    client.println("User-Agent: ArduinoWiFi/1.0");
    client.println("Connection: close");
    client.println();
    
    Serial.println("Request sent, waiting for response...");
    
    // Wait with longer timeout (10 seconds)
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 10000) {
        Serial.println("ERROR: Response timeout!");
        client.stop();
        return;
      }
    }
    
    Serial.println("Response received:");
    
    // Dump everything for debugging
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    
    client.stop();
    Serial.println("\nConnection closed");
  } else {
    Serial.println("ERROR: Connection to server failed");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
  }
}