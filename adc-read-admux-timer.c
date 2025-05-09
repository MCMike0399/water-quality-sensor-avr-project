#include <avr/io.h>
#include <avr/interrupt.h>

// Definición de pines ADC 
#define TURBIDITY_ADC 0  // A0 
#define PH_ADC 2        // A2 
#define CONDUCT_ADC 4   // A4

// Bandera para el timer
volatile bool timer1_flag = false;

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
  
  // Configurar Timer1 (ASM)
  asm volatile(
    // Configurar Timer1 en modo CTC con OCR1A como TOP
    "ldi r16, 0x00      \n"  // Modo CTC (WGM12=1, otros bits WGM=0)
    "sts %0, r16        \n"  // TCCR1A = 0
    "ldi r16, 0x0C      \n"  // WGM12=1, prescaler 256
    "sts %1, r16        \n"  // TCCR1B = 0x0C
    
    // Valor para 1 segundo: 16MHz/256 = 62500 (ticks por segundo)
    "ldi r16, 0x24      \n"  // OCR1A = 62500 (0xF424) byte alto
    "ldi r17, 0xF4      \n"  // OCR1A byte bajo
    "sts %3, r17        \n"  // OCR1AL
    "sts %2, r16        \n"  // OCR1AH

    // Habilitar interrupción de comparación A
    "ldi r16, 0x02      \n"  // OCIE1A = 1 (bit 1)
    "sts %4, r16        \n"  // TIMSK1 = 0x02
    :
    : "n" (_SFR_MEM_ADDR(TCCR1A)),
      "n" (_SFR_MEM_ADDR(TCCR1B)),
      "n" (_SFR_MEM_ADDR(OCR1AH)),
      "n" (_SFR_MEM_ADDR(OCR1AL)),
      "n" (_SFR_MEM_ADDR(TIMSK1))
    : "r16", "r17"
  );
  
  // Habilitar interrupciones globales
  sei();
}

// Interrupción del Timer1
ISR(TIMER1_COMPA_vect) {
  timer1_flag = true;
}

// Función para leer ADC (ASM)
uint16_t read_adc(uint8_t channel) {
  // Configuramos canal ADC en C
  ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
  
  uint16_t adc_value;
  
  // Start conversion and read result in assembly
  asm volatile(
    "ldi r16, 0xC7      \n"  // Incia conversión (ADSC)
    "sts %1, r16        \n"
    "wait:              \n"
    "lds r16, %1        \n"
    "sbrc r16, 6        \n"  // Espera hasta ADSC=0
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
float ph_cal_slope = 0.018;        // (14-0)/(770-0) = 0.018
float ph_cal_offset = 0.0;         // ADC=0 → pH=0

// Conductividad: ADC 0-500 → 0-1500 µS/cm
float conductividad_cal_slope = 3.0;     // (1500-0)/(500-0) = 3.0
float conductividad_cal_offset = 0.0;    // ADC=0 → 0 µS/cm

void loop() {
  // Leer los 3 sensores
  uint16_t turbidez_raw = read_adc(TURBIDITY_ADC);
  uint16_t ph_raw = read_adc(PH_ADC);
  uint16_t conductividad_raw = read_adc(CONDUCT_ADC);
  
  // Aplicar calibración para convertir a unidades correctas
  float turbidez_ntu = turbidez_raw * turbidez_cal_slope + turbidez_cal_offset;
  float ph_value = ph_raw * ph_cal_slope + ph_cal_offset;
  float conductividad_us = conductividad_raw * conductividad_cal_slope + conductividad_cal_offset;
  
  // Aplicar límites a los valores
  turbidez_ntu = constrain(turbidez_ntu, 0, 1000);
  ph_value = constrain(ph_value, 0, 14);
  conductividad_us = constrain(conductividad_us, 0, 1500);
  
  // Enviar datos calibrados
  Serial.print("T:");
  Serial.print(turbidez_ntu, 2);     // NTU con 2 decimales
  Serial.print(";PH:");
  Serial.print(ph_value, 2);         // pH con 2 decimales
  Serial.print(";C:");
  Serial.println(conductividad_us, 2); // µS/cm con 2 decimales
  
  // Esperar 1 segundo usando Timer1
  timer1_flag = false;
  while (!timer1_flag) {
    // Esperar a que timer1_flag sea true
  }
}