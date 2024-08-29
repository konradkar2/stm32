#include "comms.h"
#include "core/crc8.h"
#include "core/uart.h"

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

void log_packet(const struct comms_packet *packet)
{
	char *type_str = "UNKNOWN";
	switch (packet->type) {
	case comms_packet_type_data:
		type_str = "DATA";
		break;
	case comms_packet_type_ack:
		type_str = "ACK";
		break;
	case comms_packet_type_retx:
		type_str = "RETX";
		break;
	}

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

static void setup_static_packet(struct comms_packet *packet, enum comms_packet_type type)
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
	setup_static_packet(&retx_packet, comms_packet_type_retx);
	setup_static_packet(&ack_packet, comms_packet_type_ack);
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
				comms_write(comms, &retx_packet);
				comms->state = comms_state_length;
				break;
			}

			switch (pkt->type) {
			case comms_packet_type_retx: {
				comms->stats.pkts_retx_cnt++;
				comms_write(comms, &comms->last_write_packet);
				comms->state = comms_state_length;
			} break;
			case comms_packet_type_ack: {
				comms->stats.pkts_ack_cnt++;
				comms->state = comms_state_length;
			} break;
			case comms_packet_type_data: {
				comms->stats.pkts_data_cnt++;
				bool can_be_stored =
				    ring_buffer_get_left_space_len(&comms->packet_rb) >=
				    sizeof(struct comms_packet);

				if (!can_be_stored) {
					comms->stats.buffer_full_cnt++;
					comms_write(
					    comms,
					    &retx_packet); // not sure if this is a good
							   // idea, could make an interrupt "loop"
				} else {
					ring_buffer_write_many(&comms->packet_rb, (uint8_t *)pkt,
							       sizeof(struct comms_packet));

					comms_write(comms, &ack_packet);
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

void comms_write(struct comms *comms, struct comms_packet *packet)
{
	uart_write(comms->uart_drv, (uint8_t *)packet, sizeof(struct comms_packet));
}

void comms_read(struct comms *comms, struct comms_packet *packet)
{
	if (comms_packet_available(comms)) {
		ring_buffer_read_many(&comms->packet_rb, (uint8_t *)packet,
				      sizeof(struct comms_packet));
	}
}

