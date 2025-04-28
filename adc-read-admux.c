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

void loop() {
  // Leer los 3 sensores
  uint16_t turbidez = read_adc(TURBIDITY_ADC);
  uint16_t ph_value = read_adc(PH_ADC);
  uint16_t conductividad = read_adc(CONDUCT_ADC);

  // Enviar datos por serial (C)
  Serial.print("T:");
  Serial.print(turbidez);
  Serial.print(";PH:");
  Serial.print(ph_value);
  Serial.print(";C:");
  Serial.println(conductividad);
  
  delay(1000);  // Esperar 1 segundo
}