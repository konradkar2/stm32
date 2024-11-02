BOOTLOADER_SIZE = 0x10000
BOOOTLOADER_FILE = "bootloader.bin"

with open(BOOOTLOADER_FILE, "rb") as f:
    raw_file = f.read()

if len(raw_file) > BOOTLOADER_SIZE:
    raise "bootloader binary is too large, can't pad"

bytes_to_pad = BOOTLOADER_SIZE - len(raw_file)
print("padding bootloader of original size {} to {}".format(
      len(raw_file), BOOTLOADER_SIZE))

padding = bytes([0xff for _ in range(bytes_to_pad)])

with open(BOOOTLOADER_FILE, "wb") as f:
    f.write(raw_file + padding)
