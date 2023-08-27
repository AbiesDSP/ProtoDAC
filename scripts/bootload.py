import serial.tools.list_ports
from pathlib import Path
import subprocess
import time


def find_headphone_dac(pid=0xF232):
    ports = list(serial.tools.list_ports.comports())

    return [port.device for port in ports if port.pid == pid]


build_config = "Release"
upgrade_file = (
    Path(f"HeadphoneDAC.cydsn/CortexM3/ARM_GCC_541/{build_config}")
    / "HeadphoneDAC.cyacd"
)

ATTEMPTS = 3
WAIT_DELAY = 1


def main():
    matching = find_headphone_dac()
    if len(matching) == 0:
        raise Exception("Headphone DAC Bootloader is not running!")
    if len(matching) > 1:
        raise Exception("Multiple ports found")

    bootload_cmd = ["cyflash", f"--serial={matching[0]}", upgrade_file]

    subprocess.run(bootload_cmd, check=True)


if __name__ == "__main__":
    main()
