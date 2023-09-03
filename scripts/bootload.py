from pathlib import Path
import subprocess
from avril import Avril, find_headphone_dac
import struct
import time
import sys

import buildp

build_config = "Release"
upgrade_file = (
    Path(f"HeadphoneDAC.cydsn/CortexM3/ARM_GCC_541/{build_config}")
    / "HeadphoneDAC.cyacd"
)

ATTEMPTS = 3
WAIT_DELAY = 1

ENTER_BOOTLOAD_ADDR = 16384
BOOTLOADER_SECURITY_KEY = "0x424344454647"


def main():
    if "--build" in sys.argv:
        buildp.main()
    # Enter bootload command.
    with Avril() as av:
        av.write(ENTER_BOOTLOAD_ADDR, 42)
        time.sleep(0.1)

    time.sleep(1.0)
    # Find the port
    matching = find_headphone_dac()
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
