#!/usr/bin/env python3

"""
Script to bootload the Headphone DAC with new firmware.
This will send a command to the device to run "Bootloadable_Load()".
Then, cyflash is used as the bootloader host tool.
This will find the port based on the PID.
"""
import subprocess
from avril import Avril, find_headphone_dac
import time
import argparse
from psoc_creator import PSoCConfig
from pathlib import Path

BOOTLOADER_SECURITY_KEY = "0x424344454647"

parser = argparse.ArgumentParser("Update the ProtoDAC's firmware.")
parser.add_argument(
    "--release",
    action="store",
    default=".",
    help="Bootload from a release.",
)
parser.add_argument(
    "--dev", action="store_true", help="Use the release in the project dir."
)
parser.add_argument(
    "--config",
    action="store",
    default="ProjectConfig.yaml",
    help="Specify a config file.",
)


class Args:
    release: str
    dev: bool
    config: str


def main():
    args: Args = parser.parse_args()

    cfg = PSoCConfig.from_yaml_file(args.config)

    if args.dev:
        # Use the upgrade file in the project dir.
        upgrade_file = Path(cfg.upgrade_file)
    else:
        # Use the specified release dir, or look in the current directory.
        upgrade_file = Path(args.release) / Path(cfg.upgrade_file).name

    # Enter bootload command.
    with Avril(cfg.usb_pid) as av:
        av.write(cfg.registers["enter_bootload"], 42)
        time.sleep(0.1)

    time.sleep(1.0)
    # Find the port
    matching = find_headphone_dac(cfg.usb_pid)
    if len(matching) == 0:
        raise Exception("Headphone DAC Bootloader is not running!")
    if len(matching) > 1:
        raise Exception("Multiple ports found")
    
    # Run the bootload command.
    bootload_cmd = [
        "cyflash",
        f"--serial={matching[0]}",
        upgrade_file,
        "--timeout=1.0",
        "--psoc5",
        f"--key={BOOTLOADER_SECURITY_KEY}",
    ]

    print(f"Bootloading Upgrade File: {upgrade_file}")
    boot_attemps = 3
    while boot_attemps > 0:
        try:
            subprocess.run(bootload_cmd, check=True)
            boot_attemps = 0
            break
        except:
            boot_attemps -= 1
            if boot_attemps == 0:
                raise Exception("Too many failed bootloads")
        time.sleep(1.0)


if __name__ == "__main__":
    main()
