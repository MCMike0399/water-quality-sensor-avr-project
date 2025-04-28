# Monitor de Calidad de Agua en Tiempo Real

Este proyecto implementa un sistema de monitoreo de calidad de agua en tiempo real que recoge datos de sensores conectados a un Arduino y los muestra en una interfaz web interactiva con alertas.

## Características

- Lectura de múltiples sensores: turbidez, pH y conductividad
- Visualización en tiempo real mediante WebSockets
- Gráficos dinámicos con Plotly.js
- Sistema de alertas para valores críticos
- Interfaz web responsiva

## Estructura del Proyecto

```
proyecto/
├── water_monitor_server.py     # Servidor FastAPI principal
├── ws-client.html              # Interfaz web del monitor
├── adc-read-admux.c            # Código Arduino para leer sensores
└── README.md                   # Este archivo
```

## Requisitos

- Python 3.7+
- FastAPI
- uvicorn
- pyserial
- Arduino con sensores conectados

## Instalación

1. Instala las dependencias:
```bash
pip install fastapi uvicorn pyserial
```

2. Todos los archivos deben estar en la raíz del proyecto.

3. Sube el código Arduino al microcontrolador.

## Configuración

1. Ajusta el puerto serie en `water_monitor_server.py`:
```python
PORT = '/dev/cu.usbmodem101'  # Cambia esto a tu puerto correcto
```

## Ejecución

1. Inicia el servidor:
```bash
python water_monitor_server.py
```

2. Accede a la interfaz web:
```
http://localhost:8081
```

## Sistema de Alertas

El sistema mostrará alertas visuales en la interfaz web en las siguientes condiciones:

- **Alerta crítica (rojo)**: 
  - pH < 2 (extremadamente ácido)
  - pH > 12 (extremadamente alcalino)
  - Conductividad > 3000 μS/cm

- **Advertencia (amarillo)**:
  - pH entre 2-3 o 11-12
  - Conductividad entre 2000-3000 μS/cm

## Conexiones Arduino

- Sensor de turbidez: Pin A0
- Sensor de pH: Pin A1
- Sensor de conductividad: Pin A2

## Funcionamiento

1. El código Arduino lee los sensores y envía los datos por el puerto serie.
2. El servidor Python recibe y procesa estos datos.
3. La interfaz web se actualiza en tiempo real mediante WebSockets.
4. Si algún valor excede los umbrales establecidos, se muestra una alerta visual.

## Personalización

Puedes ajustar los umbrales de alerta modificando el objeto `THRESHOLDS` en el archivo `ws-client.html`.