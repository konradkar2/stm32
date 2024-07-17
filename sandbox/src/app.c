#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#define LED_PORT (GPIOB)
#define LED_RED_PIN (GPIO14)
#define LED_BLUE_PIN (GPIO7)

#define CPU_FREQ 216000000
#define SYSTICK_FREQ 1000

volatile uint64_t ticks = 0;

static uint64_t get_ticks(void) { return ticks; }

void sys_tick_handler(void) { ticks++; }

static void rcc_setup(void) { rcc_clock_setup_hsi(&rcc_3v3[RCC_CLOCK_3V3_216MHZ]); }

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLUE_PIN);
}

static void systic_setup(void)
{
	systick_set_frequency(SYSTICK_FREQ, CPU_FREQ);
	systick_counter_enable();
  systick_interrupt_enable();
}

// static void delay_cycles(uint32_t cycles)
// {
// 	for (uint32_t i = 0; i < cycles; i++) {
// 		__asm__("nop");
// 	}
// }

int main(void)
{
	rcc_setup();
	gpio_setup();
  systic_setup();

	uint64_t start_time = get_ticks();
	while (1) {
		if (get_ticks() - start_time >= 5000) {
			gpio_toggle(LED_PORT, LED_RED_PIN);
			gpio_toggle(LED_PORT, LED_BLUE_PIN);
      start_time = get_ticks();
		}
	}

	// Never return
	return 0;
}
