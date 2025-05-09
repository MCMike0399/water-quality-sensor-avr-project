// Código adaptado para Arduino UNO R4 WiFi (RA4M1)
// Medición de conductividad con pH y turbidez simulados

// Pin analógico para el sensor de conductividad
#define CONDUCT_ADC A4  // Pin analógico A4

// Parámetros de calibración para conductividad: ADC 0-500 → 0-1500 µS/cm
float conductividad_cal_slope = 3.0;    // (1500-0)/(500-0) = 3.0
float conductividad_cal_offset = 0.0;   // ADC=0 → 0 µS/cm

void setup() {
  Serial.begin(9600);
  analogReference();  // Usar referencia de voltaje predeterminada (5V o 3.3V según la placa)
  
  // Esperar a que se establezca la conexión serial
  delay(1000);
  Serial.println("Sistema de monitoreo de agua iniciado");
  Serial.println("Turbidez y pH simulados, conductividad real");
}

// Función para leer ADC usando analogRead estándar (no ASM)
uint16_t read_adc(uint8_t channel) {
  return analogRead(channel);
}

void loop() {
  // Leer solo el sensor de conductividad real
  uint16_t conductividad_raw = read_adc(CONDUCT_ADC);
  
  // Valores simulados para pH y turbidez (siempre 0)
  float turbidez_ntu = 0.0;
  float ph_value = 0.0;
  
  // Aplicar calibración para convertir conductividad a µS/cm
  float conductividad_us = conductividad_raw * conductividad_cal_slope + conductividad_cal_offset;
  
  // Aplicar límite a la conductividad
  conductividad_us = constrain(conductividad_us, 0, 1500);
  
  // Enviar datos por serial (mismo formato que antes)
  Serial.print("T:");
  Serial.print(turbidez_ntu, 2);  // NTU con 2 decimales (siempre 0.00)
  Serial.print(";PH:");
  Serial.print(ph_value, 2);      // pH con 2 decimales (siempre 0.00)
  Serial.print(";C:");
  Serial.println(conductividad_us, 2); // µS/cm con 2 decimales
  
  delay(1000);  // Esperar 1 segundo
}