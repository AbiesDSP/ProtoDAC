from ftdi import open_d2xx
import sys


def main():
    """Send data and receive an echo"""
    args = sys.argv[1:]
    HEADER_SIZE = 19

    with open_d2xx(baud=3.0e6, read_timeout=1) as ftdi:
        for msg in args:
            msg_bytes = msg.encode()
            ftdi.write(msg_bytes)

            resp = ftdi.read(len(msg) + HEADER_SIZE)
            print(resp.decode())


if __name__ == "__main__":
    main()
