#include "core/logger.h"
#include "core/uart.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

static struct uart_driver s_uart_logger_driver = {
    .usart_dev	     = USART2,
    .usart_clock_dev = RCC_USART2,
    .nvic_irq	     = NVIC_USART2_IRQ,
    .gpio_pins	     = GPIO5,
    .gpio_port	     = GPIOD,
    .gpio_port_clk   = RCC_GPIOD,
    .gpio_af	     = GPIO_AF7,
    .baud_rate	     = 115200,
    .mode	     = USART_MODE_TX,
};

static int uart_write_ifc(struct _reent *reent, void *cookie, const char *data, int data_len)
{
	(void)reent;
	struct uart_driver *drv = cookie;

	for (int i = 0; i < data_len; ++i) {
		uart_write_byte(drv, data[i]);
		if (data[i] == '\n') {
			uart_write_byte(drv, '\r');
		}
	}

	return data_len; // Return number of bytes written
}

static FILE uart_stream_cfg = {
    ._write  = uart_write_ifc,
    ._read   = NULL,
    ._flags  = __SWR | __SNBF,		     // OK to write | UNBUFFERED
    ._cookie = (void *)&s_uart_logger_driver // Pass your driver instance
};

FILE *create_logger(void)
{
	uart_setup(&s_uart_logger_driver);
	return &uart_stream_cfg;
}

void destroy_logger(void)
{
	uart_terminate(&s_uart_logger_driver);
}
