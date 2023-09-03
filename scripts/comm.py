from contextlib import contextmanager
import serial
import serial.tools.list_ports


def find_headphone_dac(pid=0xF149):
    """Find the device by its PID."""
    ports = list(serial.tools.list_ports.comports())

    return [port.device for port in ports if port.pid == pid]


@contextmanager
def open_serial_port(pid=0xF149, **kwargs):
    """"""
    port = find_headphone_dac(pid)[0]
    dev = serial.Serial(port, **kwargs)
    try:
        # dev.open()
        dev.flush()
        yield dev
    finally:
        dev.close()
