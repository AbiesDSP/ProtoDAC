from contextlib import contextmanager
import ftd2xx
import serial


@contextmanager
def open_d2xx(
    baud: int = 3.0e6,
    latency: int = 16,
    read_timeout: float = 1.0,
    write_timeout: float = 1.0,
):
    dev = ftd2xx.open()
    try:
        dev.setBaudRate(int(baud))
        dev.setLatencyTimer(int(latency))
        dev.setTimeouts(int(read_timeout * 1000), int(write_timeout * 1000))
        yield dev
    finally:
        dev.close()


@contextmanager
def open_port(port: str = "COM6"):
    """"""
    dev = serial.Serial(port, timeout=1.0)
    try:
        # dev.open()
        yield dev
    finally:
        dev.close()
