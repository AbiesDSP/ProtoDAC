from comm import open_serial_port
import argparse
from command import Command


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

    with open_serial_port() as dev:
        cmd = Command(1, 42, b"\x01")
        dev.write(cmd.pack())


if __name__ == "__main__":
    main()
