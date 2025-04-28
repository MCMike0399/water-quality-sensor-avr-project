import serial
import time
import json
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware 
import threading
import asyncio

# Configuración Serial
PORT = '/dev/cu.usbmodem101'  # Ajusta al puerto correcto
BAUD_RATE = 9600

# Configuración FastAPI
app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Permite todos los orígenes
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
websocket_connections = []

# HTML mejorado con gráficos Plotly
html = """<!DOCTYPE html>
<html>
    <head>
        <title>Monitor de Calidad de Agua</title>
        <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
        <style>
            body { 
                font-family: Arial, sans-serif; 
                margin: 0; 
                padding: 20px; 
                background-color: #f5f5f5;
            }
            h1 { 
                color: #2c3e50; 
                text-align: center;
                margin-bottom: 30px;
            }
            .container {
                max-width: 1200px;
                margin: 0 auto;
            }
            .status {
                padding: 8px 15px;
                border-radius: 4px;
                display: inline-block;
                margin-bottom: 15px;
                font-weight: bold;
            }
            .connected { 
                background-color: #dff0d8; 
                color: #3c763d; 
            }
            .disconnected { 
                background-color: #f2dede; 
                color: #a94442; 
            }
            .panel {
                background-color: white;
                border-radius: 8px;
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                padding: 20px;
                margin-bottom: 20px;
            }
            .readings {
                display: grid;
                grid-template-columns: repeat(3, 1fr);
                gap: 20px;
                margin-bottom: 30px;
            }
            .reading-card {
                background-color: white;
                border-radius: 8px;
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                padding: 20px;
                text-align: center;
            }
            .reading-value {
                font-size: 28px;
                font-weight: bold;
                margin: 10px 0;
            }
            .reading-label {
                color: #7f8c8d;
                font-size: 16px;
            }
            .chart-container {
                display: grid;
                grid-template-columns: 1fr;
                gap: 20px;
            }
            .chart {
                height: 350px;
                background-color: white;
                border-radius: 8px;
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                padding: 20px;
            }
            @media (max-width: 768px) {
                .readings {
                    grid-template-columns: 1fr;
                }
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>Monitor de Calidad de Agua</h1>
            <div class="panel">
                <div id="connection" class="status disconnected">Desconectado</div>
                <div id="lastUpdate"></div>
            </div>
            
            <div class="readings">
                <div class="reading-card">
                    <div class="reading-label">Turbidez</div>
                    <div id="turbidity" class="reading-value">--</div>
                    <div class="reading-label">NTU</div>
                </div>
                <div class="reading-card">
                    <div class="reading-label">pH</div>
                    <div id="ph" class="reading-value">--</div>
                    <div class="reading-label">Unidades pH</div>
                </div>
                <div class="reading-card">
                    <div class="reading-label">Conductividad</div>
                    <div id="conductivity" class="reading-value">--</div>
                    <div class="reading-label">μS/cm</div>
                </div>
            </div>
            
            <div class="chart-container">
                <div class="chart" id="combinedChart"></div>
            </div>
        </div>

        <script>
            // Configuración para almacenar historial de datos
            const MAX_DATA_POINTS = 50;
            const chartData = {
                time: [],
                turbidity: [],
                ph: [],
                conductivity: []
            };

            // Inicializar gráficos
            function initCharts() {
                // Gráfico combinado con 3 ejes Y
                const combinedTraces = [
                    {
                        x: chartData.time,
                        y: chartData.turbidity,
                        name: 'Turbidez',
                        type: 'scatter',
                        line: {color: '#3498db', width: 2},
                        yaxis: 'y'
                    },
                    {
                        x: chartData.time,
                        y: chartData.ph,
                        name: 'pH',
                        type: 'scatter',
                        line: {color: '#e74c3c', width: 2},
                        yaxis: 'y2'
                    },
                    {
                        x: chartData.time,
                        y: chartData.conductivity,
                        name: 'Conductividad',
                        type: 'scatter',
                        line: {color: '#2ecc71', width: 2},
                        yaxis: 'y3'
                    }
                ];

                const combinedLayout = {
                    title: 'Monitoreo en Tiempo Real',
                    showlegend: true,
                    legend: {
                        orientation: 'h',
                        y: 1.1
                    },
                    margin: {
                        l: 60,
                        r: 60,
                        t: 80,
                        b: 80
                    },
                    xaxis: {
                        title: {
                            text: 'Tiempo',
                            standoff: 25
                        },
                        showgrid: true,
                        zeroline: false
                    },
                    yaxis: {
                        title: 'Turbidez (NTU)',
                        titlefont: {color: '#3498db'},
                        tickfont: {color: '#3498db'},
                        side: 'left',
                        showgrid: false,
                        zeroline: false
                    },
                    yaxis2: {
                        title: 'pH',
                        titlefont: {color: '#e74c3c'},
                        tickfont: {color: '#e74c3c'},
                        side: 'right',
                        showgrid: false,
                        zeroline: false,
                        overlaying: 'y',
                        range: [0, 14]
                    },
                    yaxis3: {
                        title: 'Conductividad (μS/cm)',
                        titlefont: {color: '#2ecc71'},
                        tickfont: {color: '#2ecc71'},
                        side: 'right',
                        showgrid: false,
                        zeroline: false,
                        overlaying: 'y',
                        position: 0.85
                    }
                };

                Plotly.newPlot('combinedChart', combinedTraces, combinedLayout);
            }

            // Función para actualizar gráficos con nuevos datos
            function updateCharts(data) {
                const now = new Date();
                const timeStr = now.toLocaleTimeString();
                
                // Convertir valores para gráficos (aplicar las mismas conversiones)
                const turbidity = (data.T / 1023) * 100;
                const ph = (data.PH / 1023) * 14;
                const conductivity = data.C * 10;
                
                // Añadir nuevo punto de datos (ya convertidos)
                chartData.time.push(timeStr);
                chartData.turbidity.push(turbidity);
                chartData.ph.push(ph);
                chartData.conductivity.push(conductivity);
                
                // Limitar el número de puntos
                if (chartData.time.length > MAX_DATA_POINTS) {
                    chartData.time.shift();
                    chartData.turbidity.shift();
                    chartData.ph.shift();
                    chartData.conductivity.shift();
                }
                
                // Actualizar gráfico combinado
                Plotly.update('combinedChart', {
                    x: [chartData.time, chartData.time, chartData.time],
                    y: [chartData.turbidity, chartData.ph, chartData.conductivity]
                }, {}, [0, 1, 2]);
            }

            // Interpretar valores (convertir valores crudos ADC)
            function interpretValues(data) {
                // Convertir ADC a valores reales
                const rawTurbidity = data.T;
                const rawPh = data.PH;
                const rawConductivity = data.C;
                
                // Aplicar fórmulas de conversión
                const turbidity = (rawTurbidity / 1023) * 100;
                const ph = (rawPh / 1023) * 14;
                const conductivity = rawConductivity * 10;
                
                return {
                    T: turbidity.toFixed(2),
                    PH: ph.toFixed(2),
                    C: conductivity.toFixed(0)
                };
            }

            // Actualizar interfaz con nuevos valores
            function updateInterface(data) {
                const interpretedData = interpretValues(data);
                
                // Actualizar indicadores
                document.getElementById('turbidity').textContent = interpretedData.T;
                document.getElementById('ph').textContent = interpretedData.PH;
                document.getElementById('conductivity').textContent = interpretedData.C;
                
                // Actualizar timestamp
                const now = new Date();
                document.getElementById('lastUpdate').textContent = 
                    `Última actualización: ${now.toLocaleTimeString()}`;
                
                // Actualizar gráficos
                updateCharts(data);
            }

            // Conectar WebSocket
            function connectWebSocket() {
                const statusElement = document.getElementById('connection');
                
                statusElement.textContent = 'Conectando...';
                
                const ws = new WebSocket('ws://' + window.location.host + '/ws');
                
                ws.onopen = function() {
                    statusElement.textContent = 'Conectado';
                    statusElement.className = 'status connected';
                    console.log('WebSocket conectado');
                };
                
                ws.onmessage = function(event) {
                    const data = JSON.parse(event.data);
                    console.log('Datos recibidos:', data);
                    updateInterface(data);
                };
                
                ws.onclose = function() {
                    statusElement.textContent = 'Desconectado - Reconectando...';
                    statusElement.className = 'status disconnected';
                    // Reconectar después de 2 segundos
                    setTimeout(connectWebSocket, 2000);
                };
                
                ws.onerror = function(err) {
                    console.error('Error en WebSocket:', err);
                    ws.close();
                };
            }
            
            // Inicializar cuando la página cargue
            window.addEventListener('load', function() {
                // Inicializar gráficos
                initCharts();
                
                // Conectar WebSocket
                connectWebSocket();
            });
        </script>
    </body>
</html>"""

# Variable para almacenar los últimos datos
latest_data = {}

@app.get("/")
async def get():
    return HTMLResponse(html)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    websocket_connections.append(websocket)
    print(f"Nueva conexión WebSocket - Total: {len(websocket_connections)}")
    
    # Enviar datos actuales inmediatamente si existen
    if latest_data:
        await websocket.send_json(latest_data)
    
    try:
        while True:
            # Mantener la conexión abierta
            await asyncio.sleep(1)
    except Exception as e:
        print(f"WebSocket desconectado: {e}")
    finally:
        if websocket in websocket_connections:
            websocket_connections.remove(websocket)
        print(f"Conexión WebSocket cerrada - Quedan: {len(websocket_connections)}")

async def broadcast_data(data):
    global latest_data
    latest_data = data
    
    if not websocket_connections:
        print("No hay conexiones WebSocket activas")
        return
        
    print(f"Enviando datos a {len(websocket_connections)} conexiones")
    
    # Crear una copia para iterar de forma segura
    connections = websocket_connections.copy()
    for connection in connections:
        try:
            await connection.send_json(data)
        except Exception as e:
            print(f"Error al enviar datos: {e}")
            if connection in websocket_connections:
                websocket_connections.remove(connection)

def serial_reader(main_loop):  
    try:
        ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
        print(f"Conectado a {PORT} a {BAUD_RATE} baudios")
        
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    try:
                        # Parsear datos del formato T:valor;PH:valor;C:valor
                        data = dict(item.split(":") for item in line.split(";") if ":" in item)
                        data = {k: float(v) for k, v in data.items()}
                        
                        # Usar el loop principal para enviar datos
                        asyncio.run_coroutine_threadsafe(
                            broadcast_data(data), 
                            main_loop
                        )
                        
                        print(f"Datos procesados: {data}")
                    except Exception as e:
                        print(f"Error procesando datos: {e}")
            
            time.sleep(0.1)
    except Exception as e:
        print(f"Error en conexión serial: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

@app.on_event("startup")
async def startup_event():
    # Obtener el loop principal y pasarlo al hilo serial
    main_loop = asyncio.get_running_loop()
    threading.Thread(
        target=serial_reader, 
        args=(main_loop,),  # Pasar el loop como argumento
        daemon=True
    ).start()

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8081)