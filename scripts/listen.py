from ftdi import open_d2xx
import argparse


class Args:
    delim: str


parser = argparse.ArgumentParser()
parser.add_argument("--delim", default="\n", type=str)

READ_SIZE = 256


def main():
    """"""
    args = parser.parse_args(namespace=Args)
    # Special characters like \r
    if args.delim.startswith("\\"):
        args.delim = args.delim[1:]

    with open_d2xx(read_timeout=0.1) as ftdi:
        rx_data = bytearray()
        while True:
            # Append data to the buffer
            rx_data.extend(ftdi.read(READ_SIZE))
            # Split at a delimeter.
            spl = rx_data.split(args.delim.encode(), 1)
            # New, complete message.
            if len(spl) > 1:
                rx_data = spl[1]
                print(spl[0].decode(), end=args.delim)


if __name__ == "__main__":
    main()
