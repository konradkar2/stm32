#include "core/uart.h"
#include "core/ring_buffer.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stddef.h>

void uart_handle_irq(struct uart_driver *drv)
{
	const bool overrun_occurred = usart_get_flag(drv->usart_dev, USART_FLAG_ORE) == 1;
	const bool received_data    = usart_get_flag(drv->usart_dev, USART_FLAG_RXNE) == 1;

	if (received_data || overrun_occurred) {
		if (ring_buffer_write(&drv->rb, usart_recv(drv->usart_dev))) {
			// handle buffer overflow
		}
	}
}

void uart_setup(struct uart_driver *drv)
{
	ring_buffer_setup(&drv->rb, drv->rb_buffer, sizeof(drv->rb_buffer));

	rcc_periph_clock_enable(drv->gpio_port_clk);
	gpio_mode_setup(drv->gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, drv->gpio_pins);
	gpio_set_af(drv->gpio_port, drv->gpio_af, drv->gpio_pins);

	rcc_periph_clock_enable(drv->usart_clock_dev);
	usart_set_mode(drv->usart_dev, drv->mode);
	usart_set_flow_control(drv->usart_dev, USART_FLOWCONTROL_NONE);
	usart_set_databits(drv->usart_dev, 8);
	usart_set_baudrate(drv->usart_dev, drv->baud_rate);
	usart_set_parity(drv->usart_dev, 0);
	usart_set_stopbits(drv->usart_dev, 1);

	if ((drv->mode & USART_MODE_RX) == USART_MODE_RX) {

		usart_enable_rx_interrupt(drv->usart_dev);
		nvic_enable_irq(drv->nvic_irq);
	}

	usart_enable(drv->usart_dev);
}

void uart_terminate(struct uart_driver *drv)
{
	usart_disable(drv->usart_dev);

	if ((drv->mode & USART_MODE_RX) == USART_MODE_RX) {

		usart_disable_rx_interrupt(drv->usart_dev);
		nvic_disable_irq(drv->nvic_irq);
	}
	
	rcc_periph_clock_disable(drv->usart_clock_dev);
	gpio_mode_setup(drv->gpio_port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, drv->gpio_pins);
	rcc_periph_clock_disable(drv->gpio_port_clk);
}

void uart_write(struct uart_driver *drv, uint8_t *data, const uint32_t length)
{
	for (size_t i = 0; i < length; ++i) {
		uart_write_byte(drv, data[i]);
	}
}

void uart_write_byte(struct uart_driver *drv, uint8_t data)
{
	usart_send_blocking(drv->usart_dev, (uint16_t)data);
}

uint32_t uart_read(struct uart_driver *drv, uint8_t *data, const uint32_t length)
{
	if (length == 0) {
		return 0;
	}

	for (size_t i = 0; i < length; ++i) {
		if (!ring_buffer_read(&drv->rb, &data[i])) {
			return i;
		}
	}

	return length;
}

uint8_t uart_read_byte(struct uart_driver *drv)
{
	uint8_t byte = 0;
	uart_read(drv, &byte, 1);

	return byte;
}

bool uart_data_available(struct uart_driver *drv)
{
	return !ring_buffer_empty(&drv->rb);
}
