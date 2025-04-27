#include <avr/io.h>  // Incluir definiciones de registros AVR

void setup() {
  Serial.begin(9600);

  // Configurar ADC (Canal A0, referencia AVcc)
  asm volatile(
    "ldi r16, 0x40      \n"  // REFS0 = 1 (AVcc), canal ADC0 (A0)
    "sts %0, r16        \n"  // %0 = dirección de ADMUX
    "ldi r16, 0x87      \n"  // ADEN + ADPS2:0 = 128 (prescaler)
    "sts %1, r16        \n"  // %1 = dirección de ADCSRA
    : 
    : "n" (_SFR_MEM_ADDR(ADMUX)),  // Macro para obtener dirección de ADMUX
      "n" (_SFR_MEM_ADDR(ADCSRA))  // Macro para dirección de ADCSRA
    : "r16"  // Indicar que r16 se modifica
  );
}

void loop() {
  uint16_t adc_value;

  // Leer ADC
  asm volatile(
    "ldi r16, 0xC7      \n"  // ADSC + ADEN + prescaler 128
    "sts %1, r16        \n"  // %1 = dirección de ADCSRA
    "wait:              \n"
    "lds r16, %1        \n"  // Cargar ADCSRA en r16
    "sbrc r16, 6        \n"  // Verificar bit ADSC (bit 6)
    "rjmp wait          \n"
    "lds %A0, %2        \n"  // Leer ADCL
    "lds %B0, %3        \n"  // Leer ADCH
    : "=r" (adc_value)       // Salida: adc_value = (ADCH << 8) | ADCL
    : "n" (_SFR_MEM_ADDR(ADCSRA)),  // %1 = ADCSRA
      "n" (_SFR_MEM_ADDR(ADCL)),    // %2 = ADCL
      "n" (_SFR_MEM_ADDR(ADCH))     // %3 = ADCH
    : "r16"  // Clobber
  );

  Serial.print("ADC: ");
  Serial.println(adc_value);
  delay(1000);
}