#include "core/system.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define LED_PORT (GPIOB)
#define LED_RED_PIN (GPIO14)
#define LED_BLUE_PIN (GPIO7)

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLUE_PIN);
}

// static void delay_cycles(uint32_t cycles)
// {
// 	for (uint32_t i = 0; i < cycles; i++) {
// 		__asm__("nop");
// 	}
// }

int main(void)
{
	system_setup();
	gpio_setup();

	uint64_t start_time = system_get_ticks();
	while (1) {
		if (system_get_ticks() - start_time >= 1000) {
			gpio_toggle(LED_PORT, LED_RED_PIN);
			gpio_toggle(LED_PORT, LED_BLUE_PIN);
			start_time = system_get_ticks();
		}
	}

	// Never return
	return 0;
}
