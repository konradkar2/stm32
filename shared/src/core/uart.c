#include "core/uart.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <stddef.h>

#define BAUD_RATE 115200
#define UART_DEV USART3

static uint8_t data_buffer    = 0U;
static bool    data_available = false;
#define LED_PORT (GPIOB)
#define LED_BLUE_PIN (GPIO7)

// irq handler
void usart3_isr(void)
{
    gpio_toggle(LED_PORT, LED_BLUE_PIN);
	const bool overrun_occurred = usart_get_flag(UART_DEV, USART_FLAG_ORE) == 1;
	const bool received_data    = usart_get_flag(UART_DEV, USART_FLAG_RXNE) == 1;

	if (received_data || overrun_occurred) {
		data_buffer    = (uint8_t)usart_recv(UART_DEV);
		data_available = true;
	}
}

void uart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART3);
	usart_set_mode(UART_DEV, USART_MODE_TX_RX);
	usart_set_flow_control(UART_DEV, USART_FLOWCONTROL_NONE);
	usart_set_databits(UART_DEV, 8);
	usart_set_baudrate(UART_DEV, BAUD_RATE);
	usart_set_parity(UART_DEV, 0);
	usart_set_stopbits(UART_DEV, 1);

	usart_enable_rx_interrupt(UART_DEV);
	nvic_enable_irq(NVIC_USART3_IRQ);

	usart_enable(UART_DEV);
}

void uart_write(uint8_t *data, const uint32_t length)
{
	for (size_t i = 0; i < length; ++i) {
		uart_write_byte(data[i]);
	}
}

void uart_write_byte(uint8_t data)
{
	usart_send_blocking(UART_DEV, (uint16_t)data);
}

uint32_t uart_read(uint8_t *data, const uint32_t length)
{
	if (length > 0 && data_available) {
		*data	       = data_buffer;
		data_available = false;
		return 1;
	}

	return 0;
}

uint8_t uart_read_byte(void) {
    data_available = false;
    return data_buffer;
}

bool uart_data_available(void) {
    return data_available;
}
