import serial
import time
import json
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware 
import threading
import asyncio

# Configuración Serial
PORT = '/dev/cu.usbmodem11401'
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

# HTML de prueba para cliente WebSocket
html = """
<!DOCTYPE html>
<html>
    <head>
        <title>Monitor de Calidad de Agua</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }
            h1 { color: #333; }
            #data { 
                padding: 15px; 
                background-color: #f0f0f0; 
                border-radius: 5px; 
                margin-top: 20px;
                font-size: 18px;
                line-height: 1.5;
            }
            .status {
                padding: 5px 10px;
                border-radius: 3px;
                display: inline-block;
                margin-bottom: 10px;
            }
            .connected { background-color: #dff0d8; color: #3c763d; }
            .disconnected { background-color: #f2dede; color: #a94442; }
        </style>
    </head>
    <body>
        <h1>Monitor de Calidad de Agua</h1>
        <div id="connection" class="status disconnected">Desconectado</div>
        <div id="data">Esperando datos...</div>
        <script>
            function connectWebSocket() {
                const statusElement = document.getElementById('connection');
                const dataElement = document.getElementById('data');
                
                statusElement.textContent = 'Conectando...';
                
                const ws = new WebSocket('ws://' + window.location.host + '/ws');
                
                ws.onopen = function() {
                    statusElement.textContent = 'Conectado';
                    statusElement.className = 'status connected';
                };
                
                ws.onmessage = function(event) {
                    const data = JSON.parse(event.data);
                    dataElement.innerHTML = `
                        <strong>Turbidez:</strong> ${data.T}<br>
                        <strong>pH:</strong> ${data.PH}<br>
                        <strong>Conductividad:</strong> ${data.C}
                    `;
                    console.log('Datos recibidos:', data);
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
            
            // Iniciar conexión cuando la página cargue
            window.addEventListener('load', connectWebSocket);
        </script>
    </body>
</html>
"""

# Variable para almacenar los últimos datos
latest_data = {}

# Crear un loop de eventos para el hilo serial
loop = None

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
            # Mantener la conexión abierta y esperar mensajes
            # (esto no es necesario para la funcionalidad principal)
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