import time
import serial
import readchar
import struct
from enum import Enum

serial_dev = "/dev/ttyACM0"

# comms_packet format
comms_packet_len = 19
comms_packet_format = "B B 16s B"
comms_packet_format_crc = "B B 16s"


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


def send_packet(ser, packet_bytes):
    ser.write(packet_bytes)


class Direction(Enum):
    RX = 1
    TX = 2

class SyncSeq(Enum):
    SYNC_SEQ_0 = 0x11
    SYNC_SEQ_1 = 0x22
    SYNC_SEQ_2 = 0x33
    SYNC_SEQ_3 = 0x44


class PacketType(Enum):
    data = 0
    ack = 1
    retx = 2
    seq_observed = 3
    fw_update_req = 4
    fw_update_res = 5
    device_id_req = 6
    device_id_res = 7
    fw_length_req = 8
    fw_length_res = 9
    ready_for_data = 10
    fw_update_successful = 11
    fw_update_aborted = 12
    
    def __str__(self):
        return str(self.value)


class Packet:
    def __init__(self):
        self.dir = Direction.TX
        self.pkt = {
            "length": 16,
            "type": 0,  # data
            "data": bytes([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF]),
            "crc": 0,  # calculate later
        }

    @staticmethod
    def deserialize(bytes):
        packet = Packet()
        unpacked_data = struct.unpack(comms_packet_format, bytes)
        packet.dir = Direction.RX
        packet.pkt = {
            "length": unpacked_data[0],
            "type": unpacked_data[1],
            "data": unpacked_data[2],
            "crc": unpacked_data[3],
        }
        return packet

    def serialize(self):
        bytes = struct.pack(
            comms_packet_format,
            self.pkt["length"],
            self.pkt["type"],
            self.pkt["data"],
            self.pkt["crc"],
        )

        return bytes

    def calculate_crc(self):
        packed_data = struct.pack(
            comms_packet_format_crc, self.pkt["length"], self.pkt["type"], self.pkt["data"]
        )
        return crc8(packed_data)

    def update_crc(self):
        self.pkt["crc"] = self.calculate_crc()

    def log(self):
        packet_type_str = str(PacketType(self.pkt["type"]))

        # Convert the data field to a hex string for readability
        data_hex = " ".join(f"{byte:02X}" for byte in self.pkt["data"])

        # Log the packet

        crc_status = "valid"
        if self.calculate_crc() != self.pkt["crc"]:
            crc_status = f"invalid, expected: 0x{self.calculate_crc():02X}"

        direction_str = ""
        if (self.dir == Direction.RX):
            direction_str = "<- (RX)"
        else:
            direction_str = "-> (TX)"

        print(f"Packet Log: {direction_str}")
        print(f"  Length: {self.pkt['length']}")
        print(f"  Type: {packet_type_str} ({self.pkt['type']})")
        print(f"  Data (hex): {data_hex}")
        print(f"  CRC: 0x{self.pkt['crc']:02X} - {crc_status}")


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
    return Packet.deserialize(received_data)


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

    rx_packet = receive_packet(ser)
    rx_packet.log()

    tx_packet = Packet()
    tx_packet.log()
    send_packet(ser, tx_packet.serialize())

    rx_packet = receive_packet(ser)
    rx_packet.log()

    tx_packet = Packet()
    for i in range(0, 10):
        print("sending packet no {}".format(i))
        tx_packet.pkt['data'] = bytes([x + 1 for x in tx_packet.pkt['data']])
        tx_packet.update_crc()
        tx_packet.log()
        send_packet(ser, tx_packet.serialize())

        rx_packet = receive_packet(ser)
        rx_packet.log()


if __name__ == "__main__":
    main()
