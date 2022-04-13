import os
import subprocess
import sys
import time

import pytest
import websocket  # type: ignore

import cradle


# Also in config.yml
SERVER_URL = "ws://localhost:41071"


def server_is_running():
    try:
        websocket.create_connection(SERVER_URL)
    except ConnectionRefusedError:
        return False
    else:
        return True


def server_start():
    deploy_dir = os.environ["CRADLE_DEPLOY_DIR"]

    return subprocess.Popen(
        [deploy_dir + "/server"], cwd=deploy_dir,
        stdout=sys.stdout, stderr=subprocess.STDOUT)


def server_stop(proc):
    proc.kill()


def server_wait_until_running():
    """
    Attempt to connect to a server, retrying for a brief period if it doesn't
    initially work.
    """
    attempts_left = 100
    while True:
        try:
            return websocket.create_connection(SERVER_URL)
        except ConnectionRefusedError:
            time.sleep(0.01)
            attempts_left -= 1
            if attempts_left == 0:
                raise


@pytest.fixture(scope='session')
def server():
    proc = None
    if not server_is_running():
        proc = server_start()
        server_wait_until_running()

    yield

    if proc:
        server_stop(proc)


@pytest.fixture(scope='session')
def session(server):
    return cradle.Session()
