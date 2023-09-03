from dataclasses import dataclass, field
import struct

from contextlib import contextmanager
import serial
import serial.tools.list_ports

AVRIL_WE = 0x80000000


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


@dataclass
class AvrilCommand:
    amount: int
    address: int
    data: bytes

    HEADER_SIZE = 8
    HEADER_FMT = "<LL"

    @classmethod
    def unpack(self, buf: bytes):
        amount, address = struct.unpack(
            AvrilCommand.HEADER_FMT, buf[: AvrilCommand.HEADER_SIZE]
        )
        dat = buf[AvrilCommand.HEADER_SIZE :]
        return AvrilCommand(amount, address, dat)

    def pack(self):
        return (
            struct.pack(AvrilCommand.HEADER_FMT, self.amount, self.address) + self.data
        )


@dataclass
class Avril:
    timeout: float = 1.0
    write_timeout: float = 1.0
    dev: serial.Serial = field(init=False)

    def __enter__(self):
        port = find_headphone_dac()[0]
        self.dev = serial.Serial(
            port, timeout=self.timeout, write_timeout=self.write_timeout
        )
        self.dev.flush()
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.dev.close()

    def write(self, address: int, dat: bytes):
        """"""
        address |= AVRIL_WE
        cmd = AvrilCommand(len(dat), address, dat)
        self.dev.write(cmd.pack())

    def read(self, address: int, amount: int):
        """"""
