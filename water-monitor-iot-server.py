import json
import os
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Request
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

# Lista para almacenar conexiones WebSocket por tipo
subscribers = []  # Clientes web
publisher = None  # Arduino (solo uno)

# Variable para almacenar los últimos datos
latest_data = {}

# Para compatibilidad con Render, usar variables de entorno
PORT = int(os.environ.get("PORT", 8080))
HOST = os.environ.get("HOST", "0.0.0.0")

@app.get("/")
async def get():
    if os.path.exists("static/ws-client.html"):
        return FileResponse("static/ws-client.html")
    return {"message": "Monitor de Calidad de Agua API"}

@app.get("/health")
async def health_check():
    """Endpoint para verificar que el servicio está funcionando"""
    return {
        "status": "healthy", 
        "subscribers": len(subscribers), 
        "publisher": "connected" if publisher else "disconnected"
    }

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    global publisher
    
    await websocket.accept()
    
    # Determinar si es publisher (Arduino) o subscriber (cliente web)
    # Los headers están disponibles después de accept()
    headers = websocket.headers
    is_publisher = "x-device-type" in headers and headers["x-device-type"] == "arduino-publisher"
    
    if is_publisher:
        # Si ya hay un publisher, desconectar el anterior
        if publisher:
            try:
                await publisher.close(code=1000, reason="New publisher connected")
            except:
                pass
        
        publisher = websocket
        print("Nuevo PUBLISHER conectado (Arduino)")
    else:
        subscribers.append(websocket)
        print(f"Nuevo SUBSCRIBER conectado - Total: {len(subscribers)}")
        
        # Enviar datos actuales al nuevo subscriber si existen
        if latest_data:
            await websocket.send_json(latest_data)
    
    try:
        while True:
            # Esperar datos del cliente
            data = await websocket.receive_text()
            try:
                # Procesar los datos recibidos
                json_data = json.loads(data)
                print(f"Datos recibidos: {json_data}")
                
                # Si vienen del publisher (Arduino), reenviarlos a todos los subscribers
                if is_publisher and "C" in json_data:
                    await broadcast_to_subscribers(json_data)
                # Si es un mensaje de control de un subscriber, procesarlo
                elif not is_publisher and "type" in json_data and json_data["type"] == "control":
                    # Implementar lógica para mensajes de control si se necesita
                    pass
            except json.JSONDecodeError:
                print(f"Error: Datos no válidos: {data}")
    except WebSocketDisconnect:
        print("WebSocket desconectado")
    except Exception as e:
        print(f"Error en WebSocket: {e}")
    finally:
        if is_publisher:
            if publisher == websocket:
                publisher = None
                print("Publisher desconectado")
        else:
            if websocket in subscribers:
                subscribers.remove(websocket)
                print(f"Subscriber desconectado - Quedan: {len(subscribers)}")

async def broadcast_to_subscribers(data):
    global latest_data
    latest_data = data
    
    if not subscribers:
        print("No hay subscribers activos")
        return
        
    print(f"Enviando datos a {len(subscribers)} subscribers")
    
    # Crear una copia para iterar de forma segura
    connections = subscribers.copy()
    for connection in connections:
        try:
            await connection.send_json(data)
        except Exception as e:
            print(f"Error al enviar datos: {e}")
            if connection in subscribers:
                subscribers.remove(connection)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host=HOST, port=PORT)