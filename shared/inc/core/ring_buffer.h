#ifndef INC_CORE_RING_BUFFER_H
#define INC_CORE_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

struct ring_buffer {
	uint8_t *buffer;
	uint32_t mask;
	uint32_t read_index;
	uint32_t write_index;
};

void ring_buffer_setup(struct ring_buffer * rb, uint8_t * buffer, uint32_t size);
bool ring_buffer_empty(struct ring_buffer * rb);
bool ring_buffer_write(struct ring_buffer * rb, uint8_t byte);
bool ring_buffer_read(struct ring_buffer * rb, uint8_t * byte);


#endif /* INC_CORE_RING_BUFFER_H */