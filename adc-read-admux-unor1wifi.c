// Incluye la librería para controlar la matriz LED en el Arduino UNO R4 WiFi
#include "Arduino_LED_Matrix.h"

// Crea un objeto para interactuar con la matriz LED
ArduinoLEDMatrix matrix;

// Arreglo para almacenar el frame actual (8 filas x 12 columnas)
byte frame[8][12] = {0};

// Definición de 6 patrones para la animación de onda que se muestra en la matriz LED
// Cada patrón representa un frame de la animación
// Onda perimetral en expansión (loader animation)
byte wave[6][8][12] = {
  { // Onda 1
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1}
  },
  // Ondas 2-6 from the original code
  { // Onda 2
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,0,0,0,0,0,0,0,0,1,0},
    {0,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // Onda 3
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,1,0,0,0,0,0,0,1,0,0},
    {0,0,1,0,0,0,0,0,0,1,0,0},
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // Onda 4
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // Onda 5
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // Onda 6
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

// Patrones para mostrar dígitos 0-9 y punto decimal en la matriz LED
byte digits[11][8][12] = {
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
  { // 1
    {0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 2
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,1,1,1,1,0,0,0,0},
    {0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 3
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 4
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,1,0,1,1,0,0,0,0,0},
    {0,0,1,0,0,1,1,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 5
    {0,0,1,1,1,1,1,1,1,0,0,0},
    {0,0,1,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,0,0,1,0,0,0},
    {0,0,1,0,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 6
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,0,0,0,0,0},
    {0,1,1,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,1,1,0,0,0,0},
    {0,1,1,0,0,0,0,1,1,0,0,0},
    {0,0,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 7
    {0,0,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,0,0,0},
    {0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 8
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // 9
    {0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,1,1,0,0,0,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0},
    {0,0,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  },
  { // Decimal point
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,0,0,0}
  }
};

// Definición de pines analógicos para los sensores
#define TURBIDITY_PIN A0    // Sensor de turbidez 
#define CONDUCT_PIN   A1    // Sensor de conductividad
#define PH_PIN        A2    // Sensor de pH

// Parámetros de calibración para convertir valores ADC a unidades físicas
float turbidez_cal_slope = -1.408;     // Pendiente para conversión
float turbidez_cal_offset = 1000.0;    // Offset para conversión
float ph_cal_slope = 0.022;
float ph_cal_offset = 70.0;
float conductividad_cal_slope = 3.0;
float conductividad_cal_offset = 0.0;

// Variables para controlar tiempos de actualización
unsigned long lastSensorUpdate = 0;      // Momento de la última lectura de sensores
unsigned long lastFrameChange = 0;       // Momento del último cambio de frame en animación
const long sensorInterval = 3000;        // Intervalo para leer sensores (3 segundos)
int frameDuration = 100;                 // Velocidad base de animación (milisegundos)
int currentWaveFrame = 0;                // Frame actual en la animación de onda
float currentConductivity = 0.0;         // Último valor de conductividad leído
bool showingValue = false;               // Indica si se está mostrando un valor

void setup() {
  // Inicializa la matriz LED
  matrix.begin();
  // Configura comunicación serial a 9600 baudios
  Serial.begin(9600);
  // Configura pines analógicos como entradas
  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(CONDUCT_PIN, INPUT);
  pinMode(PH_PIN, INPUT);
}

void loop() {
  // Obtiene el tiempo actual en milisegundos
  unsigned long currentMillis = millis();
  
  // Si no está mostrando un valor numérico, actualiza la animación de carga
  if (!showingValue) {
    // Verifica si es tiempo de actualizar el frame de animación
    if (currentMillis - lastFrameChange >= frameDuration) {
      updateLoader();
      lastFrameChange = currentMillis;
    }
  }

  // Cada 3 segundos lee los sensores y muestra los datos
  if (currentMillis - lastSensorUpdate >= sensorInterval) {
    readAndShowSensorData();
    lastSensorUpdate = currentMillis;
  }
}

// Actualiza el frame de la animación de carga y lo muestra en la matriz
void updateLoader() {
  // Copia el frame actual de la animación al buffer
  memcpy(frame, wave[currentWaveFrame], sizeof(frame));
  // Renderiza el frame en la matriz LED
  matrix.renderBitmap(frame, 8, 12);
  // Avanza al siguiente frame de forma cíclica
  currentWaveFrame = (currentWaveFrame + 1) % 6;
}

// Muestra un dígito específico en la matriz LED
void displayDigit(int digit) {
  if (digit >= 0 && digit <= 10) {
    // Copia el patrón del dígito al buffer
    memcpy(frame, digits[digit], sizeof(frame));
    // Renderiza en la matriz
    matrix.renderBitmap(frame, 8, 12);
  }
}

// Muestra una animación con los dígitos del valor de conductividad
void showConductivityAnimation(float value) {
  // Indica que está mostrando valores para evitar actualizar la animación
  showingValue = true;
  
  // Extrae dígitos individuales del valor con 2 decimales
  int intPart = (int)value;                   // Parte entera
  int decimals = (int)((value - intPart) * 100); // Parte decimal
  
  // Separa cada dígito individual
  int digit1 = intPart / 100;           // Centenas
  int digit2 = (intPart / 10) % 10;     // Decenas
  int digit3 = intPart % 10;            // Unidades
  int decimal1 = decimals / 10;         // Primera decimal
  int decimal2 = decimals % 10;         // Segunda decimal
  
  // Limpia la pantalla
  clearFrame();
  
  // Muestra cada dígito con una pequeña pausa entre ellos
  // Sólo muestra centenas si son mayores que cero
  if (digit1 > 0) {
    displayDigit(digit1);
    delay(200);
    clearFrame();
    matrix.renderBitmap(frame, 8, 12);
    delay(50);
  }
  
  // Muestra decenas, unidades, punto decimal y decimales...
  // Show tens digit
  displayDigit(digit2);
  delay(200);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  // Show ones digit
  displayDigit(digit3);
  delay(200);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  // Show decimal point
  displayDigit(10);  // Index 10 is decimal point
  delay(150);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  // Show tenths digit
  displayDigit(decimal1);
  delay(200);
  clearFrame();
  matrix.renderBitmap(frame, 8, 12);
  delay(50);
  
  // Show hundredths digit
  displayDigit(decimal2);
  delay(200);

  // Indica que ya no está mostrando valores
  showingValue = false;
}

// Lee los sensores, procesa los datos y los muestra
void readAndShowSensorData() {
  // Lee valores crudos de los sensores
  int turbidez_raw = analogRead(TURBIDITY_PIN);
  int ph_raw = analogRead(PH_PIN);
  int conductividad_raw = analogRead(CONDUCT_PIN);
  
  // Convierte a valores físicos usando las ecuaciones de calibración
  float turbidez_voltaje = (turbidez_raw * 5.0) / 1023.0;
  float turbidez_ntu = turbidez_voltaje - 1120.4 * pow(turbidez_voltaje, 2) + 5742.3 * turbidez_voltaje - 4352.9;
  float ph_value = ph_raw * ph_cal_slope + ph_cal_offset;
  float conductividad_us = conductividad_raw * conductividad_cal_slope + conductividad_cal_offset;

  // Limita los valores dentro de rangos válidos
  turbidez_ntu = constrain(turbidez_ntu, 0, 1000);
  ph_value = constrain(ph_value, 0, 14);
  conductividad_us = constrain(conductividad_us, 0, 1500);
  
  // Actualiza el valor global de conductividad
  currentConductivity = conductividad_us;
  
  // Mayor conductividad = animación más rápida (frameDuration más pequeño)
  /* 
    un map se puede ver un lambda que se aplica en cada iteración
    constrain (conductividad_us, 0, 1500) limita el valor de conductividad entre 0 y 1500
    1500, 200, 50 es el rango de valores que se asignan a frameDuration
    200 es el valor máximo y 50 el mínimo
  */
  frameDuration = map(constrain(conductividad_us, 0, 1500), 0, 1500, 200, 50);
  
  // Muestra animación con el valor de conductividad
  showConductivityAnimation(conductividad_us);
  
  // Envía datos por puerto serial con formato T:valor;PH:valor;C:valor
  Serial.print("T:");
  Serial.print(turbidez_ntu, 2);
  Serial.print(";PH:");
  Serial.print(ph_value, 2);
  Serial.print(";C:");
  Serial.println(conductividad_us, 2);
}

// Limpia el buffer de frame (apaga todos los LEDs)
void clearFrame() {
  memset(frame, 0, sizeof(frame[0][0]) * 8 * 12);
}