import argparse
from avril import Avril
import time
import struct


def main():
    """"""
    with Avril() as av:
        dat = struct.pack("<L", int(time.time()))
        av.write(256, dat)


if __name__ == "__main__":
    main()
