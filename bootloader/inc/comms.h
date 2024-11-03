#ifndef INC_COMMS_H
#define INC_COMMS_H

#include "core/ring_buffer.h"
#include "core/uart.h"
#include <stdint.h>
#include <stdio.h>

#define PACKET_DATA_LEN 16
#define PACKET_RB_LEN	256

enum comms_packet_type {
	comms_packet_type_data		     = 0,
	comms_packet_type_ack		     = 1,
	comms_packet_type_retx		     = 2,
	comms_packet_type_seq_observed	     = 3,
	comms_packet_type_fw_update_req	     = 4,
	comms_packet_type_fw_update_res	     = 5,
	comms_packet_type_device_id_req	     = 6,
	comms_packet_type_device_id_res	     = 7,
	comms_packet_type_fw_length_req	     = 8,
	comms_packet_type_fw_length_res	     = 9,
	comms_packet_type_ready_for_firmware = 10,
	comms_packet_type_update_successful  = 11,
	comms_packet_type_fw_update_aborted  = 12,
	comms_packet_type_unknown	     = 13,
	comms_packet_type_max		     = 14,
};
const char *comms_packet_type_str(enum comms_packet_type);

struct comms_packet {
	uint8_t length;
	uint8_t type;
	uint8_t data[PACKET_DATA_LEN];
	uint8_t crc;
};
void log_packet(const struct comms_packet *packet);

enum comms_state_t {
	comms_state_length,
	comms_state_type,
	comms_state_data,
	comms_state_crc,
};

struct comms_stats {
	uint64_t buffer_full_cnt;
	uint64_t crc_bad_cnt;
	uint64_t tx_packets_cnt[comms_packet_type_max];
	uint64_t rx_packets_cnt[comms_packet_type_max];
};

struct comms;
void comms_print_stats(const struct comms *comms);

struct comms {
	struct uart_driver *uart_drv;
	enum comms_state_t  state;
	uint8_t		    data_idx;
	struct comms_packet packet_buffer;
	struct comms_packet last_write_packet;
	uint8_t		    packet_rb_buffer[PACKET_RB_LEN];
	struct ring_buffer  packet_rb;
	struct comms_stats  stats;
};

void comms_setup(struct comms *comms, struct uart_driver *uart_drv);
void comms_update(struct comms *comms);
bool comms_packet_available(struct comms *comms);
void comms_send(struct comms *comms, struct comms_packet *packet);
void comms_send_control_packet(struct comms *comms, enum comms_packet_type type);
void comms_receive(struct comms *comms, struct comms_packet *packet);

uint8_t comms_compute_crc(const struct comms_packet *packet);

#endif /* INC_COMMS_H */
