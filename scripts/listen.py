from ftdi import open_d2xx
import argparse


class Args:
    delim: str


parser = argparse.ArgumentParser()
parser.add_argument("--delim", default="\n", type=str)


def main():
    """"""
    args = parser.parse_args(namespace=Args)
    # Special characters like \r
    if args.delim.startswith("\\"):
        args.delim = args.delim[1:]

    with open_d2xx(read_timeout=0.01) as ftdi:
        expected_size = 32
        rx_data = bytearray()
        while True:
            # Append data to the buffer
            rx_bytes = ftdi.read(expected_size)
            if len(rx_bytes) > 0:
                expected_size = len(rx_bytes)
            rx_data.extend(rx_bytes)
            # Split at a delimeter.
            spl = rx_data.split(args.delim.encode(), 1)
            # New, complete message.
            if len(spl) > 1:
                rx_data = spl[1]
                print(spl[0].decode(), end=args.delim)


if __name__ == "__main__":
    main()
