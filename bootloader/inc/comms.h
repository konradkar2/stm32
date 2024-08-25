#ifndef INC_COMMS_H
#define INC_COMMS_H

#include "core/ring_buffer.h"
#include "core/uart.h"
#include <stdint.h>
#include <stdio.h>

#define PACKET_DATA_LEN 16
#define PACKET_RB_LEN	256

enum comms_packet_type {
	comms_packet_type_data = 0,
	comms_packet_type_ack  = 1,
	comms_packet_type_retx = 2,
};

struct comms_packet {
	uint8_t length;
	uint8_t type;
	uint8_t data[PACKET_DATA_LEN];
	uint8_t crc;
};

enum comms_state_t {
	comms_state_length,
	comms_state_type,
	comms_state_data,
	comms_state_crc,
};

struct comms_stats {
	uint64_t write_fails_cnt;
};

struct comms {
	struct uart_driver *uart_drv;
	enum comms_state_t  state;
	uint8_t		    state_bytes_count;
	struct comms_packet packet_buffer;
	struct comms_packet last_write_packet;
	uint8_t		    packet_rb_buffer[PACKET_RB_LEN];
	struct ring_buffer  packet_rb;
	struct comms_stats  stats;
};

void comms_setup(struct comms *comms, struct uart_driver *uart_drv);
void comms_update(struct comms *comms);
bool comms_packet_available(struct comms *comms);
void comms_write(struct comms *comms, struct comms_packet *packet);
void comms_read(struct comms *comms, struct comms_packet *packet);

void comms_dump_statas(struct comms *comms, FILE * file);

#endif /* INC_COMMS_H */