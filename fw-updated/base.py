import time
import serial
import readchar
import struct

serial_dev = "/dev/ttyACM0"

# comms_packet format
comms_packet_len = 19
comms_packet_format = "B B 16s B"
comms_packet_format_crc = "B B 16s"

# Example of data to send
ack_packet = {
    "length": 16,
    "type": 1,  # ack
    "data": bytes([0xFF] * 16),  # pad
    "crc": 0,  # calculate later
}

data_packet = {
    "length": 16,
    "type": 0,  # data
    "data": bytes([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF]),
    "crc": 0,  # calculate later
}


def crc8(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x07
            else:
                crc <<= 1
        crc &= 0xFF  # Ensure crc remains a uint8_t

    return crc


def packet_crc8(packet):
    packed_data = struct.pack(
        comms_packet_format_crc, packet["length"], packet["type"], packet["data"]
    )
    return crc8(packed_data)


def receive_packet(ser: serial.Serial):
    bytes_to_read = ser.in_waiting

    while bytes_to_read < comms_packet_len:
        bytes_to_read = ser.in_waiting
        time.sleep(0.1)

    received_data = ser.read(comms_packet_len)
    if len(received_data) != comms_packet_len:
        print(
            "received only {} bytes out of {}".format(
                len(received_data), comms_packet_len
            )
        )

    unpacked_data = struct.unpack(comms_packet_format, received_data)
    return {
        "length": unpacked_data[0],
        "type": unpacked_data[1],
        "data": unpacked_data[2],
        "crc": unpacked_data[3],
    }


def send_packet(ser, packet, calculate_crc=True):
    # Pack the struct data into byte
    if calculate_crc:
        packet["crc"] = packet_crc8(packet)

    print("sending packet")
    log_packet(packet)

    bytes = struct.pack(
        comms_packet_format,
        packet["length"],
        packet["type"],
        packet["data"],
        packet["crc"],
    )
    ser.write(bytes)


def log_packet(packet):
    packet_type_str = {0: "DATA", 1: "ACK", 2: "RETX"}.get(packet["type"], "UNKNOWN")

    # Convert the data field to a hex string for readability
    data_hex = " ".join(f"{byte:02X}" for byte in packet["data"])

    # Log the packet

    crc_status = "invalid"
    if packet_crc8(packet) == packet["crc"]:
        crc_status = "valid"

    print(f"Packet Log:")
    print(f"  Length: {packet['length']}")
    print(f"  Type: {packet_type_str} ({packet['type']})")
    print(f"  Data: {data_hex}")
    print(f"  CRC: 0x{packet['crc']:02X} - {crc_status}")


def main():
    # no need to close it as OS will do it
    ser = serial.Serial(
        port=serial_dev,
        baudrate=115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
    )

    if not ser.is_open:
        print("failed to open serial port {}".format(serial_dev))
        exit(1)

    print("{} opened successfuly".format(serial_dev))

    packet = receive_packet(ser)
    log_packet(packet)

    send_packet(ser, data_packet, calculate_crc=False)
    packet = receive_packet(ser)
    log_packet(packet)

    for i in range(0,10):
        print("sending packet no {}".format(i))

        data_packet['data'] = bytes([x + 1 for x in data_packet['data']])
        send_packet(ser, data_packet, calculate_crc=True)
        packet = receive_packet(ser)
        log_packet(packet)


if __name__ == "__main__":
    main()
