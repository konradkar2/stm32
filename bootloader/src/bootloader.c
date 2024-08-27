#include "core/system.h"
#include <core/logger.h>
#include <core/uart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stdint.h>
#include <stdio.h>

#include "comms.h"

// #define BOOTLOADER_SIZE 0x10000U
// #define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)

// static void go_to_app_main(void)
// {
// 	vector_table_t * vector_table = (vector_table_t * )MAIN_APP_START_ADDRESS;
// 	vector_table->reset();
// }

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

int main(void)
{
	system_setup();
	uart_setup(&s_uart_firmware_io);
	stdout = create_logger();

	struct comms comms = {0};
	comms_setup(&comms, &s_uart_firmware_io);

	printf("BOOTLOADER hello world!!!\n");

	struct comms_packet packet = {
	    .length = 9,
	    .type   = comms_packet_type_data,
	    .data   = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	    .crc    = 0,
	};

	packet.crc = comms_compute_crc(&packet);

	comms_write(&comms,&packet);
	while (true) {
		comms_update(&comms);
	}

	return 0;
}
