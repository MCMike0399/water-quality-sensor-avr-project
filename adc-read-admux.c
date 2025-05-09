#include <avr/io.h>

// Definición de pines ADC
#define TURBIDITY_ADC 0  // A0
#define PH_ADC        2  // A2
#define CONDUCT_ADC   4  // A4

void setup() {
  Serial.begin(9600);

  // Configurar ADC (ASM)
  asm volatile(
    "ldi r16, 0x40      \n"  // REFS0 = 1 (AVcc), canal inicial A0
    "sts %0, r16        \n"
    "ldi r16, 0x87      \n"  // ADEN + prescaler 128
    "sts %1, r16        \n"
    : 
    : "n" (_SFR_MEM_ADDR(ADMUX)),
      "n" (_SFR_MEM_ADDR(ADCSRA))
    : "r16"
  );
}

// Función para leer ADC (ASM)
uint16_t read_adc(uint8_t channel) {
  // Set the ADC channel in C code
  ADMUX = (ADMUX & 0xF8) | (channel & 0x07); // Clear channel bits and set new channel
  
  uint16_t adc_value;
  
  // Start conversion and read result in assembly
  asm volatile(
    "ldi r16, 0xC7      \n"  // Start conversion (ADSC)
    "sts %1, r16        \n"
    "wait:              \n"
    "lds r16, %1        \n"
    "sbrc r16, 6        \n"  // Wait until ADSC=0
    "rjmp wait          \n"
    "lds %A0, %2        \n"  // Read ADCL
    "lds %B0, %3        \n"  // Read ADCH
    : "=r" (adc_value)
    : "n" (_SFR_MEM_ADDR(ADCSRA)),
      "n" (_SFR_MEM_ADDR(ADCL)),
      "n" (_SFR_MEM_ADDR(ADCH))
    : "r16"
  );
  
  return adc_value;
}

// Parámetros de calibración basados en las especificaciones del sensor
// Turbidez: ADC 0-710 (invertido) → NTU 0-1000
float turbidez_cal_slope = -1.408;  // (0-1000)/(710-0) = -1.408
float turbidez_cal_offset = 1000.0; // ADC=0 → NTU=1000

// pH: ADC 0-770 → pH 0-14
float ph_cal_slope = 0.022;        // (14-0)/(700-70) = 0.018
float ph_cal_offset = 70.0;         // ADC=0 → pH=0

// Conductividad: ADC 0-500 → 0-1500 µS/cm
float conductividad_cal_slope = 3.0;     // (1500-0)/(500-0) = 3.0
float conductividad_cal_offset = 0.0;    // ADC=0 → 0 µS/cm

void loop() {
  // Leer los 3 sensores
  uint16_t turbidez_raw = read_adc(TURBIDITY_ADC);
  uint16_t ph_raw = read_adc(PH_ADC);
  uint16_t conductividad_raw = read_adc(CONDUCT_ADC);
  float turbidez_voltaje = (turbidez_raw * 5.0) / 1023.0; // Convertir a voltaje

  // Aplicar calibración para convertir a unidades correctas
  float turbidez_ntu = turbidez_voltaje - 1120.4 * turbidez_voltaje * turbidez_voltaje + 5742.3 * turbidez_voltaje - 4352.9; // Fórmula de calibración
  float ph_value = ph_raw * ph_cal_slope + ph_cal_offset;
  float conductividad_us = conductividad_raw * conductividad_cal_slope + conductividad_cal_offset;

  // Aplicar límites a los valores
  turbidez_ntu = constrain(turbidez_ntu, 0, 1000);
  ph_value = constrain(ph_value, 0, 14);
  conductividad_us = constrain(conductividad_us, 0, 1500);

  // Enviar datos calibrados
  Serial.print("T:");
  Serial.print(turbidez_ntu, 2);  // NTU con 2 decimales
  Serial.print(";PH:");
  Serial.print(ph_value, 2);      // pH con 2 decimales 
  Serial.print(";C:");
  Serial.println(conductividad_us, 2); // µS/cm con 2 decimales
  
  delay(1000);  // Esperar 1 segundo
}