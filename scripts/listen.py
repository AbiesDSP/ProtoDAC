from ftdi import open_d2xx
from contextlib import contextmanager
import argparse


parser = argparse.ArgumentParser()
parser.add_argument("--delim", default="\n", type=str)


class Args:
    delim: str


def main():
    """"""
    args = parser.parse_args(namespace=Args)
    if args.delim.startswith("\\"):
        args.delim = args.delim[1:]

    with open_d2xx(read_timeout=0.1) as ftdi:
        rx_data = bytearray()
        while True:
            READ_SIZE = 32
            rx_data.extend(ftdi.read(READ_SIZE))
            spl = rx_data.split(args.delim.encode(), 1)
            # New line
            if len(spl) > 1:
                rx_data = spl[1]
                print(spl[0].decode(), end=args.delim)


if __name__ == "__main__":
    main()
