BOOTLOADER_SIZE = 0x8000
BOOOTLOADER_FILE = "bootloader.bin"

with open(BOOOTLOADER_FILE, "rb") as f:
    raw_file = f.read()

bytes_to_pad = BOOTLOADER_SIZE - len(raw_file);
padding = bytes([0xff for _ in range(bytes_to_pad)])

with open(BOOOTLOADER_FILE, "wb") as f:
    f.write(raw_file + padding)


