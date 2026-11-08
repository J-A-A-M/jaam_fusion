import subprocess
from pathlib import Path


def test_ws_backoff(tmp_path):
    exe = tmp_path / "ws_backoff"
    src = Path(__file__).with_name("test_ws_backoff.cpp")
    subprocess.run(["g++", "-std=c++11", "-o", str(exe), str(src)], check=True)
    subprocess.run([str(exe)], check=True)
