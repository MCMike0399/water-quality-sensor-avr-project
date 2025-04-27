import serial
import time
import json
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware 
import threading
import asyncio

# Configuración Serial
PORT = '/dev/cu.usbmodem1401'
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
    </head>
    <body>
        <h1>Datos en tiempo real</h1>
        <div id="data"></div>
        <script>
            const ws = new WebSocket('ws://localhost:8000/ws');
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                document.getElementById('data').innerHTML = `
                    Turbidez: ${data.T}<br>
                    pH: ${data.PH}<br>
                    Conductividad: ${data.C}
                `;
            };
        </script>
    </body>
</html>
"""

@app.get("/")
async def get():
    return HTMLResponse(html)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    websocket_connections.append(websocket)
    try:
        while True:
            await asyncio.sleep(1)  # Mantener conexión activa
    except:
        websocket_connections.remove(websocket)

async def broadcast_data(data):
    for connection in websocket_connections[:]:  # Iterar sobre copia
        try:
            await connection.send_json(data)
        except:
            websocket_connections.remove(connection)

def serial_reader():
    ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
    print(f"Conectado a {PORT}...")
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line:
                data = {}
                parts = line.split(';')
                for part in parts:
                    key, value = part.split(':')
                    data[key] = value
                
                # Convertir a JSON y broadcast
                asyncio.run(broadcast_data(data))
                print(f"Datos enviados: {data}")

        time.sleep(0.1)

if __name__ == "__main__":
    # Iniciar hilo para lectura serial
    threading.Thread(target=serial_reader, daemon=True).start()
    
    # Iniciar servidor FastAPI
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)