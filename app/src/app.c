#include "core/system.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "core/timer.h"
#include <libopencm3/cm3/scb.h>

#define LED_PORT (GPIOB)
#define LED_RED_PIN (GPIO14)
#define LED_BLUE_PIN (GPIO7)


#define BOOTLOADER_SIZE 0x8000U

static void vector_setup(void)
{
	SCB_VTOR = BOOTLOADER_SIZE;
}

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	// gpio_mode_setup(LED_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LED_RED_PIN);
	// gpio_set_af(LED_PORT, GPIO_AF9, LED_RED_PIN);

	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLUE_PIN);
}


int main(void)
{
	vector_setup();
	system_setup();
	gpio_setup();
	timer_setup();

	uint64_t blue_time = system_get_ticks();
	//uint64_t red_time = system_get_ticks();
	//float duty_cycle = 100;

	//timer_pwm_set_duty_cycle(duty_cycle);
	while (1) {
		if (system_get_ticks() - blue_time >= 300) {
			gpio_toggle(LED_PORT, LED_BLUE_PIN);
			blue_time = system_get_ticks();
		}

		// if (system_get_ticks() - red_time >= 30) {
		// 	duty_cycle -= 1;
		// 	if(duty_cycle <= 0) {
		// 		duty_cycle = 100;
		// 	}
		// 	timer_pwm_set_duty_cycle(duty_cycle);
		// 	red_time = system_get_ticks();
		// }
	}

	// Never return
	return 0;
}
