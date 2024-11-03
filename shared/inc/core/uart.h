#ifndef INC_CORE_UART_H
#define INC_CORE_UART_H

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "ring_buffer.h"
#include <stdbool.h>
#include <stdint.h>

struct uart_driver {
	uint32_t	      usart_dev;
	enum rcc_periph_clken usart_clock_dev;
	uint8_t	      nvic_irq;
    uint32_t gpio_pins;
    uint32_t gpio_port;
    uint32_t gpio_af;
    enum rcc_periph_clken gpio_port_clk;
	uint32_t	      baud_rate;
    uint32_t mode;

	// for 115200 Bd/s, (we have 1 symbol per 1 bod, that is 0 or 1)
	// each 10 symbols represent single byte
	// so it's actually 11520 bytes/s
	// that gives us ~11.5(bytes/ms)
	// this can hold up to 11ms of data coming to UART without read
	// 128B / 11.5(B/ms)  = 11ms
	uint8_t		   rb_buffer[128];
	struct ring_buffer rb;
};

void uart_setup(struct uart_driver *drv);
void uart_terminate(struct uart_driver *drv);
void uart_handle_irq(struct uart_driver *drv);

void	 uart_write(struct uart_driver *drv, uint8_t *data, const uint32_t length);
void	 uart_write_byte(struct uart_driver *drv, uint8_t data);
uint32_t uart_read(struct uart_driver *drv, uint8_t *data, const uint32_t length);
uint8_t	 uart_read_byte(struct uart_driver *drv);
bool	 uart_data_available(struct uart_driver *drv);

#endif /* INC_CORE_UART_H */
