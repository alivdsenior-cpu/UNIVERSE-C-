import asyncio
import json
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware
import os
import sys

if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class ConnectionManager:
    def __init__(self):
        self.active_connections: list[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)

    async def broadcast(self, message: str):
        for connection in self.active_connections.copy():
            try:
                await connection.send_text(message)
            except Exception:
                self.disconnect(connection)

manager = ConnectionManager()

# Montamos la carpeta frontend para servir el HTML/CSS/JS
app.mount("/", StaticFiles(directory="frontend", html=True), name="static")

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            # Mantiene la conexión viva y escucha comandos si fuera necesario
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)

async def read_engine_output():
    exe_path = "./apextrade.exe"
    if not os.path.exists(exe_path):
        print(f"[Backend] ERROR: No se encontró el ejecutable {exe_path}. ¿Fue compilado?")
        return

    print("[Backend] Iniciando Motor HFT C++ en segundo plano...")
    process = await asyncio.create_subprocess_exec(
        exe_path,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE
    )

    try:
        while True:
            line = await process.stdout.readline()
            if not line:
                break
            
            line_str = line.decode('utf-8').strip()
            if line_str.startswith("{"):
                # Retransmitir JSON a todos los clientes web conectados en tiempo real
                await manager.broadcast(line_str)
            else:
                print(f"[ApexTrade] {line_str}")
    except Exception as e:
        print(f"[Backend] Error leyendo el motor: {e}")
    finally:
        try:
            process.terminate()
        except:
            pass

@app.on_event("startup")
async def startup_event():
    # Inicia la lectura del motor C++ como una tarea en segundo plano al arrancar FastAPI
    asyncio.create_task(read_engine_output())

if __name__ == "__main__":
    import uvicorn
    # Lanzar servidor en el puerto 8000
    uvicorn.run("server:app", host="127.0.0.1", port=8000, reload=True)
