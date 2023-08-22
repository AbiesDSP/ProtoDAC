import ftd2xx
from ftd2xx import FTD2XX

from contextlib import contextmanager
import argparse


@contextmanager
def open_d2xx(baud=3.0e6) -> FTD2XX:
    dev = ftd2xx.open()

    try:
        dev.setBaudRate(int(baud))
        dev.setLatencyTimer(2)
        dev.setTimeouts(1, 1)
        yield dev
    finally:
        dev.close()


parser = argparse.ArgumentParser()
parser.add_argument("--delim", default="\n")


def main():
    """"""
    args = parser.parse_args()
    delim = args.delim.encode()
    if delim == b"\\r":
        delim = b"\r"

    with open_d2xx() as ftdi:
        ftdi: FTD2XX

        rx_data = bytearray()
        while True:
            rx_size = ftdi.getQueueStatus()
            if rx_size > 0:
                rx_data.extend(ftdi.read(rx_size))
                spl = rx_data.split(delim, 1)
                # New line
                if len(spl) > 1:
                    rx_data = spl[1]
                    print(spl[0].decode(), end=delim.decode())


if __name__ == "__main__":
    main()
