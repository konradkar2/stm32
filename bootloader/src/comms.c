#include "comms.h"
#include "core/crc8.h"
#include "core/str.h"
#include "core/uart.h"
#include <string.h>

static struct comms_packet retx_packet = {0};
static struct comms_packet ack_packet  = {0};

uint8_t comms_compute_crc(const struct comms_packet *packet)
{
	return crc8((uint8_t *)packet, sizeof(struct comms_packet) - 1); // exclude CRC
}

void print_stats(const struct comms *comms)
{
	printf("Comms Stats:\n");
	printf("Buffer Full Count: %llu\n", comms->stats.buffer_full_cnt);
	printf("Packets DATA Count: %llu\n", comms->stats.pkts_data_cnt);
	printf("Packets ACK Count: %llu\n", comms->stats.pkts_ack_cnt);
	printf("Packets RETX Count: %llu\n", comms->stats.pkts_retx_cnt);
	printf("Packets NOK Count: %llu\n", comms->stats.pkts_nok_cnt);
	printf("Buffer space left: %lu\n", ring_buffer_get_left_space_len(&comms->packet_rb));
	printf("Buffer space taken: %lu\n", ring_buffer_get_data_len(&comms->packet_rb));
}

const char *comms_packet_type_str(enum comms_packet_type type)
{
	switch (type) {
		ENUM_CASE(comms_packet_type_data)
		ENUM_CASE(comms_packet_type_ack)
		ENUM_CASE(comms_packet_type_retx)
		ENUM_CASE(comms_packet_type_seq_observed)
		ENUM_CASE(comms_packet_type_fw_update_req)
		ENUM_CASE(comms_packet_type_fw_update_res)
		ENUM_CASE(comms_packet_type_device_id_req)
		ENUM_CASE(comms_packet_type_device_id_res)
		ENUM_CASE(comms_packet_type_fw_length_req)
		ENUM_CASE(comms_packet_type_fw_length_res)
		ENUM_CASE(comms_packet_type_ready_for_firmware)
		ENUM_CASE(comms_packet_type_update_successful)
		ENUM_CASE(comms_packet_type_fw_update_aborted)
	default:
		return "comms_packet_type_unknown";
	}
}

void log_packet(const struct comms_packet *packet)
{
	const char *type_str = comms_packet_type_str(packet->type);

	printf("Packet:\n");
	printf(" Length: %d\n", packet->length);
	printf(" Type %s: (%d)\n", type_str, packet->type);
	printf(" Data: ");
	for (int i = 0; i < PACKET_DATA_LEN; ++i) {
		printf("%02hhX ", packet->data[i]);
	}
	printf("\n");
	const bool crc_valid = comms_compute_crc(packet) == packet->crc;
	printf(" CRC: %02hhX - %s\n", packet->crc, crc_valid ? "valid" : "invalid");
}

static void comms_create_control_packet(struct comms_packet *packet, enum comms_packet_type type)
{
	packet->length = PACKET_DATA_LEN;
	packet->type   = type;
	for (int i = 0; i < PACKET_DATA_LEN; ++i) {
		packet->data[i] = 0xff;
	}
	packet->crc = comms_compute_crc(packet);
}

void comms_setup(struct comms *comms, struct uart_driver *uart_drv)
{
	comms->uart_drv = uart_drv;
	comms_create_control_packet(&retx_packet, comms_packet_type_retx);
	comms_create_control_packet(&ack_packet, comms_packet_type_ack);
	ring_buffer_setup(&comms->packet_rb, comms->packet_rb_buffer, PACKET_RB_LEN);
}

#define TRACE_LOG() printf("%s:%d", __func__, __LINE__)

void comms_update(struct comms *comms)
{

	struct comms_packet *pkt      = &comms->packet_buffer;
	struct uart_driver  *uart_drv = comms->uart_drv;

	while (uart_data_available(uart_drv)) {
		switch (comms->state) {
		case comms_state_length: {
			pkt->length  = uart_read_byte(uart_drv);
			comms->state = comms_state_type;
		} break;
		case comms_state_type: {
			pkt->type    = uart_read_byte(uart_drv);
			comms->state = comms_state_data;
		} break;
		case comms_state_data: {
			pkt->data[comms->data_idx] = uart_read_byte(uart_drv);

			comms->data_idx++;
			if (comms->data_idx >= PACKET_DATA_LEN) {
				comms->data_idx = 0;
				comms->state	= comms_state_crc;
			}
		} break;
		case comms_state_crc: {
			pkt->crc	   = uart_read_byte(uart_drv);
			uint8_t actual_crc = comms_compute_crc(pkt);

			if (pkt->crc != actual_crc) {
				comms->stats.pkts_nok_cnt++;
				comms_send(comms, &retx_packet);
				comms->state = comms_state_length;
				break;
			}

			switch (pkt->type) {
			case comms_packet_type_retx: {
				comms->stats.pkts_retx_cnt++;
				comms_send(comms, &comms->last_write_packet);
				comms->state = comms_state_length;
			} break;
			case comms_packet_type_ack: {
				comms->stats.pkts_ack_cnt++;
				comms->state = comms_state_length;
			} break;
			default: {
				comms->stats.pkts_data_cnt++;
				bool can_be_stored =
				    ring_buffer_get_left_space_len(&comms->packet_rb) >=
				    sizeof(struct comms_packet);

				if (!can_be_stored) {
					comms->stats.buffer_full_cnt++;
					comms_send(
					    comms,
					    &retx_packet); // not sure if this is a good
							   // idea, could make an interrupt "loop"
				} else {
					ring_buffer_write_many(&comms->packet_rb, (uint8_t *)pkt,
							       sizeof(struct comms_packet));

					comms_send(comms, &ack_packet);
				}
				comms->state = comms_state_length;
			}
			}
			break;

		default:
			comms->state = comms_state_length;
		}
		}
	}
}
bool comms_packet_available(struct comms *comms)
{
	return !ring_buffer_empty(&comms->packet_rb) &&
	       ring_buffer_get_data_len(&comms->packet_rb) >= sizeof(struct comms_packet);
}

void comms_send(struct comms *comms, struct comms_packet *packet)
{
	uart_write(comms->uart_drv, (uint8_t *)packet, sizeof(struct comms_packet));
	memcpy(&comms->last_write_packet, packet, sizeof(struct comms_packet));
}

void comms_send_control_packet(struct comms *comms, enum comms_packet_type type)
{
	struct comms_packet packet = {0};
	comms_create_control_packet(&packet, type);

	comms_send(comms, &packet);
}

void comms_receive(struct comms *comms, struct comms_packet *packet)
{
	if (comms_packet_available(comms)) {
		ring_buffer_read_many(&comms->packet_rb, (uint8_t *)packet,
				      sizeof(struct comms_packet));
	}
}
