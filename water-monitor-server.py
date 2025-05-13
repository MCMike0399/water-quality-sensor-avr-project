import serial
import time
import json
import os
import random
from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import threading
import asyncio

# Configuración Serial
PORT = os.environ.get('SERIAL_PORT', '/dev/cu.usbmodem101')
BAUD_RATE = int(os.environ.get('BAUD_RATE', '9600'))

# Configuración FastAPI
app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
app.mount("/static", StaticFiles(directory="static"), name="static")

# Lista para almacenar conexiones WebSocket
websocket_connections = []

# Variable para almacenar los últimos datos
latest_data = {}

@app.get("/")
async def get():
    return FileResponse("static/ws-client.html")

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
        # Entramos a modo simulado si no existe un puerto serial
        if not PORT:
            print("No serial port specified. Using simulation mode.")
            while True:
                simulated_data = {
                    "T": round(20 + 5 * random.random(), 2),
                    "PH": round(6.5 + 1.5 * random.random(), 2),
                    "C": round(300 + 200 * random.random(), 2)
                }
                asyncio.run_coroutine_threadsafe(
                    broadcast_data(simulated_data), 
                    main_loop
                )
                print(f"Simulated data: {simulated_data}")
                time.sleep(3)

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
    port = int(os.environ.get('PORT', '8080'))
    uvicorn.run(app, host="0.0.0.0", port=port)