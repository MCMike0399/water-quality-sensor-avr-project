import json
import os
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
import asyncio

# Configuración FastAPI
app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Montar archivos estáticos si el directorio existe
if os.path.exists("static"):
    app.mount("/static", StaticFiles(directory="static"), name="static")

# Lista para almacenar conexiones WebSocket
websocket_connections = []

# Variable para almacenar los últimos datos
latest_data = {}

# Para compatibilidad con Render, usar variables de entorno
PORT = int(os.environ.get("PORT", 8080))
HOST = os.environ.get("HOST", "0.0.0.0")

@app.get("/")
async def get():
    # Verificar si el archivo existe antes de intentar servirlo
    if os.path.exists("static/ws-client.html"):
        return FileResponse("static/ws-client.html")
    return {"message": "Monitor de Calidad de Agua API"}

@app.get("/health")
async def health_check():
    """Endpoint para verificar que el servicio está funcionando"""
    return {"status": "healthy"}

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
            # Esperar datos del cliente (ahora puede ser el Arduino)
            data = await websocket.receive_text()
            try:
                # Procesar los datos recibidos
                json_data = json.loads(data)
                print(f"Datos recibidos: {json_data}")
                
                # Si vienen del Arduino, reenviarlos a todos los clientes
                if "C" in json_data:
                    await broadcast_data(json_data)
            except json.JSONDecodeError:
                print(f"Error: Datos no válidos: {data}")
    except WebSocketDisconnect:
        print("WebSocket desconectado")
    except Exception as e:
        print(f"Error en WebSocket: {e}")
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

# Este bloque es crucial para Render
if __name__ == "__main__":
    import uvicorn
    uvicorn.run("water_monitor_iot_server:app", host=HOST, port=PORT, reload=False)