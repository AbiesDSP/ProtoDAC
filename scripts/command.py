from dataclasses import dataclass
import struct

# command = {length, address, data}


@dataclass
class Command:
    amount: int
    address: int
    data: bytes

    HEADER_SIZE = 8
    HEADER_FMT = "<LL"

    @classmethod
    def unpack(self, buf: bytes):
        amount, address = struct.unpack(Command.HEADER_FMT, buf[: Command.HEADER_SIZE])
        dat = buf[Command.HEADER_SIZE :]
        return Command(amount, address, dat)

    def pack(self):
        return struct.pack(Command.HEADER_FMT, self.amount, self.address) + self.data
