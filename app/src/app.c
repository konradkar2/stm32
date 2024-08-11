//most of these is taken from
// https://www.youtube.com/@LowByteProductions
// 

#include "core/system.h"
#include "timer.h"
#include <core/uart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>

#define LED_PORT     (GPIOB)
#define LED_RED_PIN  (GPIO14)
#define LED_BLUE_PIN (GPIO7)

#define BOOTLOADER_SIZE 0x8000U

static void vector_setup(void)
{
	SCB_VTOR = 0x08008000U;
}

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(LED_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LED_RED_PIN);
	gpio_set_af(LED_PORT, GPIO_AF9, LED_RED_PIN);

	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLUE_PIN);
}

static struct uart_driver s_uart_logger_driver = {.dev		 = USART2,
						  .clock_dev	 = RCC_USART2,
						  .nvic_irq	 = NVIC_USART2_IRQ,
						  .gpio_pins	 = GPIO5,
						  .gpio_port	 = GPIOD,
						  .gpio_port_clk = RCC_GPIOD,
						  .gpio_af	 = GPIO_AF7,
						  .baud_rate	 = 115200,
						  .mode		 = USART_MODE_TX};

static struct uart_driver s_uart_firmware_io = {.dev	       = USART3,
						.clock_dev     = RCC_USART3,
						.nvic_irq      = NVIC_USART3_IRQ,
						.gpio_pins     = GPIO8 | GPIO9,
						.gpio_port     = GPIOD,
						.gpio_port_clk = RCC_GPIOD,
						.gpio_af       = GPIO_AF7,
						.baud_rate     = 115200,
						.mode	       = USART_MODE_TX_RX};

void usart3_isr(void)
{
	uart_handle_irq(&s_uart_firmware_io);
}

static uint8_t write_count = 0;
static int uart_write_ifc(struct _reent *reent, void *cookie, const char *data, int data_len)
{
	write_count++;
	(void)reent;
	struct uart_driver *drv = cookie;;

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

int main(void)
{
	vector_setup();
	system_setup();
	gpio_setup();
	timer_setup();
	uart_setup(&s_uart_logger_driver);
	uart_setup(&s_uart_firmware_io);
	stdout = &uart_stream_cfg;

	printf("Hello, UART!\n");

	// uint64_t blue_time  = system_get_ticks();
	uint64_t red_time   = system_get_ticks();
	float	 duty_cycle = 100;

	for(int i = 0; i< 255 ; ++i)
		printf("%d: A message from UART logger!!\n", i);
		
	// timer_pwm_set_duty_cycle(duty_cycle);
	while (1) {
		// if (system_get_ticks() - blue_time >= 300) {
		//	gpio_toggle(LED_PORT, LED_BLUE_PIN);
		//	blue_time = system_get_ticks();
		// }

		if (system_get_ticks() - red_time >= 30) {
			duty_cycle -= 1;
			if (duty_cycle <= 0) {
				duty_cycle = 100;
			}
			timer_pwm_set_duty_cycle(duty_cycle);
			red_time = system_get_ticks();
		}

		if (uart_data_available(&s_uart_firmware_io)) {
			uint8_t data = uart_read_byte(&s_uart_firmware_io);
			uart_write_byte(&s_uart_firmware_io, data + 1);
		}

		
	}

	// Never return
	return 0;
}
