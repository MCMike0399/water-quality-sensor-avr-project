import serial
import time

PORT = '/dev/cu.usbmodem11401'  # Ajusta al puerto correcto
BAUD_RATE = 9600

try:
    ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
    print(f"Conectado a {PORT}...")
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line:
                # Parsear datos (formato: "T:123;PH:456;C:789")
                data = {}
                parts = line.split(';')
                for part in parts:
                    key, value = part.split(':')
                    data[key] = value
                
                print(f"Turbidez: {data['T']}, pH: {data['PH']}, Conductividad: {data['C']}")
                
        time.sleep(0.1)

except serial.SerialException as e:
    print(f"Error: {e}")
except KeyboardInterrupt:
    print("\nPrograma detenido.")
    ser.close()
except Exception as e:
    print(f"Error inesperado: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    print("Conexión cerrada.")