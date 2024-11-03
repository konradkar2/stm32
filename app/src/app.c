// most of these is taken from
//  https://www.youtube.com/@LowByteProductions
//

#include "core/system.h"
#include "timer.h"
#include <core/logger.h>
#include <core/simple-timer.h>
#include <core/uart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stdint.h>
#include <stdio.h>

#define LED_PORT     (GPIOB)
#define LED_RED_PIN  (GPIO14)
#define LED_BLUE_PIN (GPIO7)

static void vector_setup(void)
{
	SCB_VTOR = 0x08000000U + BOOTLOADER_SIZE;
}

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	// gpio_mode_setup(LED_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LED_RED_PIN);
	// gpio_set_af(LED_PORT, GPIO_AF9, LED_RED_PIN);

	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
}

int main(void)
{
	vector_setup();
	system_setup();
	gpio_setup();
	timer_setup();
	stdout = create_logger();

	printf("Hello, from main app!\n");

	struct simple_timer timer = {0};
	simple_timer_setup(&timer, 1000, true);

	while (1) {
		if (simple_timer_has_elapsed(&timer)) {
			gpio_toggle(LED_PORT, LED_RED_PIN);
		}
	}

	// Never return
	return 0;
}
