from comm import open_serial_port
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

    with open_serial_port(timeout=0.05) as dev:
        rx_data = bytearray()
        while True:
            # Read up to the max from the device. times out if fewer.
            rx_bytes = dev.read(64)

            # Append data to the buffer
            rx_data.extend(rx_bytes)

            # Split at a delimeter.
            spl = rx_data.split(args.delim.encode(), 1)
            # New, complete message.
            if len(spl) > 1:
                rx_data = spl[1]
                print(spl[0].decode(), end=args.delim)


if __name__ == "__main__":
    main()
