# Converts bytes to integer values uint32
def bytes_to_int(bytes, length):
        new_bytes = []
        for x in range(0, length):
                new_bytes.append(int.from_bytes(
                bytes[(x*4):(x*4+4)], byteorder='little', signed=False))
        return new_bytes

# int16
def bytes_to_int16(bytes, length):
        new_bytes = []
        for x in range(0, length):
                new_bytes.append(int.from_bytes(
                bytes[(x*2):(x*2+2)], byteorder='big', signed=True))
        return new_bytes

def bytes_to_signed_int32(bytes, length):
        new_bytes = []
        for x in range(0, length):
                new_bytes.append(int.from_bytes(
                bytes[(x*4):(x*4+4)], byteorder='little', signed=True))
        return new_bytes