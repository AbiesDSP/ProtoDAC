"""
The Headphone DAC will send log messages on USB serial port.
This script opens the serial port, sets the device's timestamp with the PC time,
then prints log messages.
"""
#!/usr/bin/env python3
"""
Listen to Log Messages
"""

import argparse
import time
from datetime import datetime
from avril import Avril
from psoc_creator import PSoCConfig


class Args:
    delim: str
    config: str


parser = argparse.ArgumentParser("Listen to Log Messages")
parser.add_argument("--delim", default="\n", type=str)
parser.add_argument("--config", action="store", default="ProjectConfig.yaml")


def parse_msg(msg: str):
    """Parse header and convert timestamp"""
    try:
        hstr, rest = msg.split("- ")
        level, source_file, ts = (
            hstr.replace("[", "").replace("]", "").strip().split(" : ")
        )

        t = datetime.fromtimestamp(float(ts))
        tfmt = t.strftime("%H:%M:%S")

        fmt_msg = f"[{level} : {source_file} : {tfmt}] - {rest}"
    except:
        fmt_msg = ""
    return fmt_msg


def main():
    """"""
    args: Args = parser.parse_args()
    cfg = PSoCConfig.from_yaml_file(args.config)

    # Special characters like \r
    if args.delim.startswith("\\"):
        args.delim = args.delim[1:]

    with Avril(timeout=0.05) as av:
        # Set the timestamp
        av.write(cfg.registers["timestamp"], int(time.time()))

        rx_data = bytearray()
        while True:
            # Read up to the max from the device. times out if fewer.
            rx_bytes = av.dev.read(64)

            # Append data to the buffer
            rx_data.extend(rx_bytes)

            # Split at a delimeter.
            spl = rx_data.split(args.delim.encode(), 1)
            # New, complete message.
            if len(spl) > 1:
                rx_data = spl[1]
                msg = spl[0].decode()
                fmt_msg = parse_msg(msg)
                print(fmt_msg, end=args.delim)


if __name__ == "__main__":
    main()
