import random
import time

import pytest


def create_calculation(seconds):
    return \
        {
            "lambda": {
                "function": "sleep",
                "args": [
                    {
                        "value": seconds
                    }
                ]
            }
        }


def test_perform_local_calc(session):
    num_calculation_threads = 6
    values = []
    for i in range(num_calculation_threads):
        values.append(28.0 + random.random() * 4.0)
    calculations = [create_calculation(value) for value in values]
    cmds = []
    for i in range(num_calculation_threads * 3):
        cmd = session.start_local_calc(calculations[i % num_calculation_threads])
        cmds.append(cmd)
        if i == 0:
            time.sleep(8.3)
        else:
            time.sleep(random.random() * 3.0)
    for cmd in cmds:
        new_calculation_result = session.receive_response(cmd)
        print(f'{new_calculation_result=}')
        assert new_calculation_result == pytest.approx(42)
