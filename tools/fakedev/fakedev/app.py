import importlib
import re
import uvicorn

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import Callable

from .protocols import *


class Control(BaseModel):
    request_type: int
    request: int
    value: int


class SendData(BaseModel):
    data: str


class App(FastAPI):
    def start(self, socket_path: str):
        uvicorn.run(self, uds=socket_path)


def empty_callback(*args, **kwargs):
    pass


def create_app(vid: int, pid: int, protocol: str, callback: Callable = empty_callback):
    app = App()

    scenarios = []
    protocol_scenarios = globals()[protocol.upper()]

    for scenario in protocol_scenarios:
        scenarios.append({
            "input": re.compile(scenario["input"]),
            "output": scenario["output"]
        })

    last_msg = None

    @app.get("/")
    async def root():
        return {"status": "ok"}

    @app.get("/vendor_id")
    async def vendor_id():
        callback("vendor_id")
        return {"vendor_id": vid}


    @app.get("/product_id")
    async def product_id():
        callback("product_id")
        return {"product_id": pid}


    @app.get("/bind")
    async def bind():
        callback("bind")
        return {}


    @app.post("/control")
    async def control(ctrl: Control):
        callback("control", **ctrl.dict())
        return {}


    @app.post("/send/{endpoint}")
    async def send(endpoint: int, data: SendData):
        nonlocal last_msg
        callback("send", endpoint=endpoint, **data.dict())
        if last_msg is not None:
            raise HTTPException(status_code=400, detail="Last message hasn't been processed yet")
        last_msg = data
        return {}

    @app.get("/recv/{endpoint}")
    async def recv(endpoint: int):
        nonlocal last_msg
        callback("recv", endpoint=endpoint)
        result = {"data": ""}
        for scenario in scenarios:
            if scenario["input"].fullmatch(last_msg.data):
                result["data"] = scenario["input"].sub(scenario["output"], last_msg.data)
        last_msg = None
        return result

    return app


def start_app(socket_path: str, vid: int, pid: int, protocol: str, callback: Callable = empty_callback):
    app = create_app(vid, pid, protocol, callback=callback)
    app.start(socket_path)
