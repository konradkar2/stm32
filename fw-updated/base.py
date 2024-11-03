import time
import serial
import readchar
import struct
import sys
from enum import Enum

serial_dev = "/dev/ttyACM0"

# comms_packet format
comms_packet_len = 19
comms_packet_format = "B B 16s B"
comms_packet_format_crc = "B B 16s"

BOOTLOADER_SIZE = 0x10000
PACKET_DATA_LEN_MAX = 16
DEVICE_ID = 0x69
SYNC_SEQ = [0x11, 0x22, 0x33, 0x44]


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


class Direction(Enum):
    RX = 1
    TX = 2


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
    ready_for_firmware = 10
    fw_update_successful = 11
    fw_update_aborted = 12
    unknown = 13

    def __str__(self):
        return str(self._name_)


class Packet:
    def __init__(self):
        self.dir = Direction.TX
        self.length = 0
        self.type = 0
        self.data = bytes(0xff for _ in range(PACKET_DATA_LEN_MAX))
        self.crc = 0

    @staticmethod
    def create_by_type(type: PacketType):
        packet = Packet()
        packet.type = type.value
        return packet

    def set_data(self, data: bytes):
        if (len(data) > PACKET_DATA_LEN_MAX):
            raise Exception("Packet data size exceeded")

        self.length = len(data)
        self.data = data

        if self.length < PACKET_DATA_LEN_MAX:
            self.data = self.data + \
                bytes(0xff for _ in range(PACKET_DATA_LEN_MAX - self.length))

    @staticmethod
    def create_ctrl_packet(type: PacketType):
        packet = Packet.create_by_type(type)
        packet.update_crc()

        return packet

    @staticmethod
    def deserialize(bytes):
        packet = Packet()
        unpacked_data = struct.unpack(comms_packet_format, bytes)
        packet.dir = Direction.RX
        packet.length = unpacked_data[0]
        packet.type = unpacked_data[1]
        packet.data = unpacked_data[2]
        packet.crc = unpacked_data[3]

        return packet

    def serialize(self):
        bytes = struct.pack(
            comms_packet_format,
            self.length,
            self.type,
            self.data,
            self.crc,
        )

        return bytes

    def calculate_crc(self):
        packed_data = struct.pack(
            comms_packet_format_crc, self.length, self.type, self.data
        )
        return crc8(packed_data)

    def update_crc(self):
        self.crc = self.calculate_crc()

    def log(self):
        packet_type_str = str(PacketType(self.type))

        # Convert the data field to a hex string for readability
        data_hex = " ".join(f"{byte:02X}" for byte in self.data)

        # Log the packet

        crc_status = "valid"
        if self.calculate_crc() != self.crc:
            crc_status = f"invalid, expected: 0x{self.calculate_crc():02X}"

        direction_str = ""
        if (self.dir == Direction.RX):
            direction_str = "<- (RX)"
        else:
            direction_str = "-> (TX)"

        print(f"Packet Log: {direction_str}")
        print(f"  Length: {self.length}")
        print(f"  Type: {packet_type_str} ({self.type})")
        print(f"  Data (hex): {data_hex}")
        print(f"  CRC: 0x{self.crc:02X} - {crc_status}")


def receive_packet_of_type(ser, packetType, timeout=2):
    packet = receive_packet(ser, 5, timeout)
    if packet.type != packetType.value:
        raise Exception(
            "failed to receive ctrl pkt of type: {}".format(str(packetType)))

    return packet


def send_packet(ser, packet):
    #print("sending {} packet".format(str(PacketType(packet.type))))
    ser.write(packet.serialize())

    if PacketType(packet.type) != PacketType.ack:
        receive_packet_of_type(ser, PacketType.ack)


def send_ack_packet(ser):
    ack_packet = Packet()
    ack_packet.type = PacketType.ack.value
    ack_packet.update_crc()

    send_packet(ser, ack_packet)


def send_retx_packet(ser):
    ack_packet = Packet()
    ack_packet.type = PacketType.retx.value
    ack_packet.update_crc()

    send_packet(ser, ack_packet)


def receive_packet(ser: serial.Serial, crc_invalid_retries=5, timeout=2):
    if crc_invalid_retries == -1:
        raise Exception("retry limit reached,aborting")

    timeout_cnt = 0

    bytes_to_read = ser.in_waiting
    while bytes_to_read < comms_packet_len:
        bytes_to_read = ser.in_waiting
        time.sleep(0.01)
        timeout_cnt += 0.01

        if (timeout_cnt > timeout):
            raise Exception("timeout on receive packet, in waiting bytes: {} out of {} expected".format(
                ser.in_waiting, comms_packet_len))

    received_data = ser.read(comms_packet_len)
    if len(received_data) != comms_packet_len:
        print(
            "received only {} bytes out of {}".format(
                len(received_data), comms_packet_len
            )
        )

    packet = Packet.deserialize(received_data)

    if packet.crc != packet.calculate_crc():
        print("invalid CRC")
        send_retx_packet(ser)
        return receive_packet(ser, crc_invalid_retries - 1)

    if (PacketType(packet.type) != PacketType.ack):
        send_ack_packet(ser)

    return packet


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

    ser.write(bytes(SYNC_SEQ))

    seq_observed_pkt = receive_packet_of_type(ser, PacketType.seq_observed)
    seq_observed_pkt.log()

    fw_update_req_pkt = Packet.create_ctrl_packet(PacketType.fw_update_req)
    fw_update_req_pkt.log()
    send_packet(ser, fw_update_req_pkt)

    fw_update_res_pkt = receive_packet_of_type(ser, PacketType.fw_update_res)
    fw_update_res_pkt.log()
    device_id_req_pkt = receive_packet_of_type(ser, PacketType.device_id_req)
    device_id_req_pkt.log()

    device_id_res_pkt = Packet.create_by_type(PacketType.device_id_res)
    device_id_res_pkt.set_data(bytes([DEVICE_ID,]))
    device_id_res_pkt.update_crc()
    send_packet(ser, device_id_res_pkt)

    fw_length_req_pkt = receive_packet_of_type(ser, PacketType.fw_length_req)
    fw_length_req_pkt.log()

    image_bytes = bytes()
    with open(sys.argv[1], "rb") as f:
        image_bytes = f.read()

    # skip bootloader bytes, we will send only actual APP
    app_bytes = image_bytes[BOOTLOADER_SIZE:]
    app_size = len(app_bytes)

    fw_length_res = Packet.create_by_type(PacketType.fw_length_res)
    fw_length_res.set_data(app_size.to_bytes(4, 'little'))
    fw_length_res.update_crc()
    send_packet(ser, fw_length_res)

    bytes_sent = 0
    data_packet = Packet.create_by_type(PacketType.data)
    

    while bytes_sent < app_size:
        fw_length_req_pkt = receive_packet_of_type(
            ser, PacketType.ready_for_firmware, 15)

        packet_data_len = PACKET_DATA_LEN_MAX
        bytes_left = app_size - bytes_sent

        if (bytes_left < PACKET_DATA_LEN_MAX):
            packet_data_len = bytes_left

        data_packet.set_data(app_bytes[bytes_sent:bytes_sent+packet_data_len])
        data_packet.update_crc()
        send_packet(ser, data_packet)

        bytes_sent = bytes_sent + packet_data_len
        print("sent {} bytes out of {}".format(bytes_sent, app_size))


if __name__ == "__main__":
    main()
