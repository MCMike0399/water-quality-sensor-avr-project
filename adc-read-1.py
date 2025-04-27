import serial
import time

PORT = '/dev/cu.usbmodem1401'  
BAUD_RATE = 9600

try:
    # Inicializar conexión serial
    ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
    print(f"Conectado al puerto {PORT}...")
    
    while True:
        if ser.in_waiting > 0:
            # Leer una línea del serial
            line = ser.readline().decode('utf-8').strip()
            print(f"Valor ADC: {line}")
            
        time.sleep(0.1)  # Pequeña pausa para no saturar la CPU

except serial.SerialException as e:
    print(f"Error de conexión: {e}")
except KeyboardInterrupt:
    print("\nPrograma detenido por el usuario.")
    ser.close()
except Exception as e:
    print(f"Error inesperado: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    print("Conexión serial cerrada.")