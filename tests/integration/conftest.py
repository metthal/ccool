import asyncio
import json
import multiprocessing
import os
import pytest
import queue
import re
import requests
import requests_unixsocket
import subprocess
import sys
import threading
import time
import urllib.parse
import yaml

from datetime import datetime, timedelta
from pathlib import Path
from typing import Callable

TESTS_ROOT_DIR = Path(__file__).parents[1]
CCOOL_ROOT_DIR = Path(__file__).parents[2]

sys.path.append(str(TESTS_ROOT_DIR))
sys.path.append(str(CCOOL_ROOT_DIR / "tools" / "fakedev"))

from fakedev.app import start_app as start_fakedev_app
from framework import Call


TEST_COMBINATIONS = []
TESTS_LOGS_DIR = TESTS_ROOT_DIR / "logs"


class FakedevChannel:
    """
    Channel between emulated device (fakedev) and the test environment.
    Starts a separate thread which collects logs from emulated device
    """
    def __init__(self, socket_path: str, device_spec: dict):
        self.socket_path = socket_path
        self.spec = device_spec
        self.message_channel = multiprocessing.Queue()
        self._collector_running = False
        self._collector_thread = None
        self._collector_new_message_lock = threading.Lock()
        self._collector_new_message = threading.Condition(self._collector_new_message_lock)
        self._message_log = []
        self._message_log_str = ""

    def start(self):
        self._collector_running = True
        self._collector_thread = threading.Thread(target=self._collect_logs)
        self._collector_thread.start()

    def stop(self):
        self._collector_running = False
        if self._collector_thread is not None:
            self._collector_thread.join()

    def ping(self):
        try:
            status = self._send_request("/")
            return status["status"] == "ok"
        except Exception as err:
            return False

    def callback(self, name: str, **kwargs):
        self.message_channel.put((name, kwargs))

    def assert_has_message_pattern(self, pattern, count=1, timeout=10, title=None):
        timeout_time = datetime.now() + timedelta(seconds=timeout)
        pattern_regex = re.compile(pattern.to_regex())
        while self._collector_running:
            if datetime.now() > timeout_time:
                break

            try:
                with self._collector_new_message:
                    self._collector_new_message.wait(timeout=1)
                    if len(pattern_regex.findall(self._message_log_str)) == count:
                        return True
            except TimeoutError:
                continue

        assert False, "Device did not receive expected message sequence{}\n" \
            "Expected sequence:\n" \
            "{}\n" \
            "\n" \
            "The received message sequence:\n" \
            "{}".format(f"for '{title}" if title else "", pattern, "\n".join([str(i) for i in self._message_log]))

    def _collect_logs(self):
        while self._collector_running:
            try:
                msg = self.message_channel.get(timeout=1)
                with self._collector_new_message:
                    self._message_log.append(msg)
                    if self._message_log_str:
                        self._message_log_str += "-"
                    self._message_log_str += str(Call(msg[0], **msg[1]))
                    self._collector_new_message.notify()
            except queue.Empty:
                continue

    def _send_request(self, resource: str):
        session = requests_unixsocket.Session()
        response = session.get(self._url(resource), timeout=1)
        response.raise_for_status()
        return response.json()

    def _url(self, resource: str):
        return f"http+unix://{urllib.parse.quote_plus(self.socket_path)}{resource}"


class CCool:
    def __init__(self, socket_path: str):
        self.socket_path = socket_path

    def ping(self):
        try:
            return len(self.run("info")) > 0
        except Exception:
            return False

    def run(self, *args):
        stdout = None
        stderr = None
        process = None

        try:
            process = subprocess.Popen(
                ["ccool", "--json", "--socket", self.socket_path, *list([str(arg) for arg in args])],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            try:
                stdout, stderr = process.communicate(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate()

            if stderr:
                raise RuntimeError("ccool exited with exit code {}\n{}".format(process.returncode, stderr))
            return json.loads(stdout)
        finally:
            test_log_dir = TESTS_LOGS_DIR / get_normalized_test_name()
            test_log_dir.mkdir(parents=True, exist_ok=True)

            with open(test_log_dir / "ccool.ec.log", "w") as f:
                f.write(str(process.returncode) if process else "")
            with open(test_log_dir / "ccool.stdout.log", "w") as f:
                f.write(stdout if stdout else "")
            with open(test_log_dir / "ccool.stderr.log", "w") as f:
                f.write(stderr if stderr else "")


def get_normalized_test_name():
    return os.environ.get("PYTEST_CURRENT_TEST").split(":")[-1].rsplit(" ", 1)[0].replace(" ", "_")


def get_ccoold_socket_path():
    return os.path.realpath(f"ccoold.{get_normalized_test_name()}.sock")


def get_fakedev_socket_path():
    return os.path.realpath(f"fakedev.{get_normalized_test_name()}.sock")


@pytest.fixture
def fakedev(request):
    """
    Fakedev fixture for providing access to emulated device in the test.
    """
    socket_path = get_fakedev_socket_path()
    if os.path.exists(socket_path):
        os.unlink(socket_path)
    fakedev_channel = FakedevChannel(socket_path, request.param)
    fakedev_process = multiprocessing.Process(
        target=start_fakedev_app,
        args=(socket_path, request.param["usb"]["vendor_id"], request.param["usb"]["product_id"], request.param["protocol"]),
        kwargs={
            "callback": fakedev_channel.callback
        }
    )

    try:
        fakedev_channel.start()
        fakedev_process.start()

        tries = 0
        while not fakedev_channel.ping():
            time.sleep(0.1)
            tries += 1
            if tries == 50:
                raise TimeoutError("Unable to contact fakedev server")

        yield fakedev_channel
    finally:
        fakedev_process.terminate()
        fakedev_process.join()
        fakedev_channel.stop()
        os.unlink(socket_path)


@pytest.fixture
def ccool():
    socket_path = get_ccoold_socket_path()
    fakedev_socket_path = get_fakedev_socket_path()
    ccoold_process = None

    try:
        ccoold_env = os.environ.copy()
        ccoold_env["CCOOLD_DEBUG_DEVICE_INTERFACE_SOCKET"] = fakedev_socket_path
        ccoold_process = subprocess.Popen(["ccoold", "-i", "debug", "-s", socket_path], env=ccoold_env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

        ccool = CCool(socket_path)

        tries = 0
        while not ccool.ping():
            time.sleep(0.1)
            tries += 1
            if tries == 50:
                raise TimeoutError("Unable to contact ccoold")

        yield ccool
    finally:
        stdout = None
        stderr = None
        test_log_dir = TESTS_LOGS_DIR / get_normalized_test_name()
        test_log_dir.mkdir(parents=True, exist_ok=True)

        if ccoold_process is not None:
            ccoold_process.terminate()
            try:
                stdout, stderr = ccoold_process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                ccoold_process.kill()
                stdout, stderr = ccoold_process.communicate()

        with open(test_log_dir / "ccoold.ec.log", "w") as f:
            f.write(str(ccoold_process.returncode) if ccoold_process else "")
        with open(test_log_dir / "ccoold.stdout.log", "w") as f:
            f.write(stdout if stdout else "")
        with open(test_log_dir / "ccoold.stderr.log", "w") as f:
            f.write(stderr if stderr else "")


def pytest_configure():
    devices_dir = CCOOL_ROOT_DIR / "specs" / "devices"
    for root, _, files in os.walk(devices_dir):
        for f in filter(lambda f: f.endswith(".yml") or f.endswith(".yaml"), files):
            with open(os.path.join(root, f), "r") as device_file:
                device_spec = yaml.load(device_file, Loader=yaml.SafeLoader)
                TEST_COMBINATIONS.append(device_spec)

    if not TEST_COMBINATIONS:
        raise pytest.UsageError("Integration tests: Unable to collect any test combinations")


def pytest_generate_tests(metafunc):
    if "fakedev" in metafunc.fixturenames:
        metafunc.parametrize(
            "fakedev",
            TEST_COMBINATIONS,
            ids=[spec["name"] for spec in TEST_COMBINATIONS],
            indirect=True
        )
