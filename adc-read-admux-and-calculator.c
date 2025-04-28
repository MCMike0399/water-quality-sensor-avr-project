#include <avr/io.h>

// Definición de pines ADC
#define TURBIDITY_ADC 0  // A0
#define PH_ADC        2  // A2
#define CONDUCT_ADC   4  // A4

// Función para convertir ADC a pH usando ASM (1-14)
uint8_t convert_to_ph(uint16_t adc_value) {
  uint8_t ph;
  
  // El siguiente bloque ASM convierte el valor ADC (0-1023) a pH (1-14)
  asm volatile(
    // Cargar valor ADC en r24:r25 (r24=LSB, r25=MSB)
    "movw r24, %1       \n"
    
    // Valores umbrales alto/bajo para comparaciones 16-bit
    
    // pH = 1 (0-73)
    "cpi r24, 74        \n"
    "cpc r25, __zero_reg__ \n"
    "brsh check_ph2     \n"
    "ldi %0, 1          \n"
    "rjmp end_ph        \n"
    
    // pH = 2 (74-146)
    "check_ph2:         \n"
    "cpi r24, 147       \n"
    "cpc r25, __zero_reg__ \n"
    "brsh check_ph3     \n"
    "ldi %0, 2          \n"
    "rjmp end_ph        \n"
    
    // pH = 3 (147-219)
    "check_ph3:         \n"
    "cpi r24, 220       \n"
    "cpc r25, __zero_reg__ \n"
    "brsh check_ph4     \n"
    "ldi %0, 3          \n"
    "rjmp end_ph        \n"
    
    // pH = 4 (220-292)
    "check_ph4:         \n"
    "cpi r24, 37        \n"  // 293 = 0x0125, byte bajo = 37 (0x25)
    "ldi r23, 1         \n"  // byte alto = 1 (0x01)
    "cpc r25, r23       \n"
    "brsh check_ph5     \n"
    "ldi %0, 4          \n"
    "rjmp end_ph        \n"
    
    // pH = 5 (293-365)
    "check_ph5:         \n"
    "cpi r24, 110       \n"  // 366 = 0x016E, byte bajo = 110 (0x6E)
    "ldi r23, 1         \n"  // byte alto = 1 (0x01)
    "cpc r25, r23       \n"
    "brsh check_ph6     \n"
    "ldi %0, 5          \n"
    "rjmp end_ph        \n"
    
    // pH = 6 (366-438)
    "check_ph6:         \n"
    "cpi r24, 183       \n"  // 439 = 0x01B7, byte bajo = 183 (0xB7)
    "ldi r23, 1         \n"  // byte alto = 1 (0x01)
    "cpc r25, r23       \n"
    "brsh check_ph7     \n"
    "ldi %0, 6          \n"
    "rjmp end_ph        \n"
    
    // pH = 7 (439-511)
    "check_ph7:         \n"
    "cpi r24, 0         \n"   // 512 = 0x0200, byte bajo = 0
    "ldi r23, 2         \n"   // byte alto = 2
    "cpc r25, r23       \n"
    "brsh check_ph8     \n"
    "ldi %0, 7          \n"
    "rjmp end_ph        \n"
    
    // pH = 8 (512-584)
    "check_ph8:         \n"
    "cpi r24, 73        \n"   // 585 = 0x0249, byte bajo = 73 (0x49)
    "ldi r23, 2         \n"   // byte alto = 2
    "cpc r25, r23       \n"
    "brsh check_ph9     \n"
    "ldi %0, 8          \n"
    "rjmp end_ph        \n"
    
    // pH = 9 (585-657)
    "check_ph9:         \n"
    "cpi r24, 146       \n"   // 658 = 0x0292, byte bajo = 146 (0x92)
    "ldi r23, 2         \n"   // byte alto = 2
    "cpc r25, r23       \n"
    "brsh check_ph10    \n"
    "ldi %0, 9          \n"
    "rjmp end_ph        \n"
    
    // pH = 10 (658-730)
    "check_ph10:        \n"
    "cpi r24, 219       \n"   // 731 = 0x02DB, byte bajo = 219 (0xDB)
    "ldi r23, 2         \n"   // byte alto = 2
    "cpc r25, r23       \n"
    "brsh check_ph11    \n"
    "ldi %0, 10         \n"
    "rjmp end_ph        \n"
    
    // pH = 11 (731-803)
    "check_ph11:        \n"
    "cpi r24, 36        \n"    // 804 = 0x0324, byte bajo = 36 (0x24)
    "ldi r23, 3         \n"    // byte alto = 3
    "cpc r25, r23       \n"
    "brsh check_ph12    \n"
    "ldi %0, 11         \n"
    "rjmp end_ph        \n"
    
    // pH = 12 (804-876)
    "check_ph12:        \n"
    "cpi r24, 109       \n"    // 877 = 0x036D, byte bajo = 109 (0x6D)
    "ldi r23, 3         \n"    // byte alto = 3
    "cpc r25, r23       \n"
    "brsh check_ph13    \n"
    "ldi %0, 12         \n"
    "rjmp end_ph        \n"
    
    // pH = 13 (877-949)
    "check_ph13:        \n"
    "cpi r24, 182       \n"    // 950 = 0x03B6, byte bajo = 182 (0xB6)
    "ldi r23, 3         \n"    // byte alto = 3
    "cpc r25, r23       \n"
    "brsh check_ph14    \n"
    "ldi %0, 13         \n"
    "rjmp end_ph        \n"
    
    // pH = 14 (950-1023)
    "check_ph14:        \n"
    "ldi %0, 14         \n"
    
    "end_ph:            \n"
    : "=r" (ph)           // Salida: pH calculado
    : "r" (adc_value)     // Entrada: valor ADC
    : "r23", "r24", "r25" // Registros modificados
  );
  
  return ph;
}

// Función para convertir ADC a nivel de turbidez (0-4)
uint8_t convert_to_turbidity(uint16_t adc_value) {
  uint8_t turbidity;
  
  asm volatile(
    // Cargar valor ADC en r24:r25
    "movw r24, %1       \n"
    
    // Nivel 0 (0-204): Agua clara
    "cpi r24, 205       \n"
    "cpc r25, __zero_reg__ \n"
    "brsh check_turb1   \n"
    "ldi %0, 0          \n"
    "rjmp end_turb      \n"
    
    // Nivel 1 (205-409): Ligeramente turbia
    "check_turb1:       \n"
    "cpi r24, 154       \n"  // 410 = 0x019A, byte bajo = 154 (0x9A)
    "ldi r23, 1         \n"  // byte alto = 1
    "cpc r25, r23       \n"
    "brsh check_turb2   \n"
    "ldi %0, 1          \n"
    "rjmp end_turb      \n"
    
    // Nivel 2 (410-614): Moderadamente turbia
    "check_turb2:       \n"
    "cpi r24, 103       \n"  // 615 = 0x0267, byte bajo = 103 (0x67)
    "ldi r23, 2         \n"  // byte alto = 2
    "cpc r25, r23       \n"
    "brsh check_turb3   \n"
    "ldi %0, 2          \n"
    "rjmp end_turb      \n"
    
    // Nivel 3 (615-819): Muy turbia
    "check_turb3:       \n"
    "cpi r24, 52        \n"   // 820 = 0x0334, byte bajo = 52 (0x34)
    "ldi r23, 3         \n"   // byte alto = 3
    "cpc r25, r23       \n"
    "brsh check_turb4   \n"
    "ldi %0, 3          \n"
    "rjmp end_turb      \n"
    
    // Nivel 4 (820-1023): Extremadamente turbia
    "check_turb4:       \n"
    "ldi %0, 4          \n"
    
    "end_turb:          \n"
    : "=r" (turbidity)    // Salida: nivel de turbidez
    : "r" (adc_value)     // Entrada: valor ADC
    : "r23", "r24", "r25" // Registros modificados
  );
  
  return turbidity;
}

// Función para convertir ADC a nivel de conductividad (0-4)
uint8_t convert_to_conductivity(uint16_t adc_value) {
  uint8_t conductivity;
  
  asm volatile(
    // Cargar valor ADC en r24:r25
    "movw r24, %1       \n"
    
    // Nivel 0 (0-204): Conductividad muy baja
    "cpi r24, 205       \n"
    "cpc r25, __zero_reg__ \n"
    "brsh check_cond1   \n"
    "ldi %0, 0          \n"
    "rjmp end_cond      \n"
    
    // Nivel 1 (205-409): Conductividad baja
    "check_cond1:       \n"
    "cpi r24, 154       \n"  // 410 = 0x019A, byte bajo = 154 (0x9A)
    "ldi r23, 1         \n"  // byte alto = 1
    "cpc r25, r23       \n"
    "brsh check_cond2   \n"
    "ldi %0, 1          \n"
    "rjmp end_cond      \n"
    
    // Nivel 2 (410-614): Conductividad media
    "check_cond2:       \n"
    "cpi r24, 103       \n"  // 615 = 0x0267, byte bajo = 103 (0x67)
    "ldi r23, 2         \n"  // byte alto = 2
    "cpc r25, r23       \n"
    "brsh check_cond3   \n"
    "ldi %0, 2          \n"
    "rjmp end_cond      \n"
    
    // Nivel 3 (615-819): Conductividad alta
    "check_cond3:       \n"
    "cpi r24, 52        \n"   // 820 = 0x0334, byte bajo = 52 (0x34)
    "ldi r23, 3         \n"   // byte alto = 3
    "cpc r25, r23       \n"
    "brsh check_cond4   \n"
    "ldi %0, 3          \n"
    "rjmp end_cond      \n"
    
    // Nivel 4 (820-1023): Conductividad muy alta
    "check_cond4:       \n"
    "ldi %0, 4          \n"
    
    "end_cond:          \n"
    : "=r" (conductivity) // Salida: nivel de conductividad
    : "r" (adc_value)     // Entrada: valor ADC
    : "r23", "r24", "r25" // Registros modificados
  );
  
  return conductivity;
}

// Función para leer ADC (ASM)
uint16_t read_adc(uint8_t channel) {
  // Configurar canal ADC
  ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
  
  uint16_t adc_value;
  
  // Iniciar conversión y leer resultado
  asm volatile(
    "ldi r16, 0xC7      \n"  // Iniciar conversión (ADSC)
    "sts %1, r16        \n"
    "wait:              \n"
    "lds r16, %1        \n"
    "sbrc r16, 6        \n"  // Esperar hasta ADSC=0
    "rjmp wait          \n"
    "lds %A0, %2        \n"  // Leer ADCL
    "lds %B0, %3        \n"  // Leer ADCH
    : "=r" (adc_value)
    : "n" (_SFR_MEM_ADDR(ADCSRA)),
      "n" (_SFR_MEM_ADDR(ADCL)),
      "n" (_SFR_MEM_ADDR(ADCH))
    : "r16"
  );
  
  return adc_value;
}

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

void loop() {
  // Leer los valores ADC
  uint16_t turbidez_raw = read_adc(TURBIDITY_ADC);
  uint16_t ph_raw = read_adc(PH_ADC);
  uint16_t conductividad_raw = read_adc(CONDUCT_ADC);
  
  // Convertir a valores significativos usando ASM
  uint8_t ph_value = convert_to_ph(ph_raw);
  uint8_t turbidez_nivel = convert_to_turbidity(turbidez_raw);
  uint8_t conductividad_nivel = convert_to_conductivity(conductividad_raw);

  // Enviar datos procesados por serial
  Serial.print("T:");
  Serial.print(turbidez_nivel);
  Serial.print(";PH:");
  Serial.print(ph_value);
  Serial.print(";C:");
  Serial.println(conductividad_nivel);
  
  delay(1000);  // Esperar 1 segundo
}