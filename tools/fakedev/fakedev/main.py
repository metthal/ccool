import json
import sys
import typer
import sys

from .app import start_app


cli = typer.Typer()


def start(vendor_id: str, product_id: str, protocol: str, socket_path: str = typer.Option("fakedev.sock", "-s", "--socket")):
    try:
        start_app(socket_path, int(vendor_id, 0), int(product_id, 0), protocol)
    except Exception as err:
        print(f"No protocol '{protocol}' found ({repr(err)})")
        sys.exit(1)


def main():
    typer.run(start)
