#include "core/system.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

volatile uint64_t ticks = 0;

// IRQ handler
void sys_tick_handler(void)
{
	ticks++;
}

uint64_t system_get_ticks(void)
{
	return ticks;
}

static void rcc_setup(void)
{
	rcc_clock_setup_hsi(&rcc_3v3[RCC_CLOCK_3V3_216MHZ]);
}

static void systic_setup(void)
{
	systick_set_frequency(SYSTICK_FREQ, CPU_FREQ);
	systick_counter_enable();
	systick_interrupt_enable();
}

void system_setup(void)
{
	rcc_setup();
	systic_setup();
}

void system_terminate(void)
{	
	systick_interrupt_disable();
	systick_counter_disable();
	systick_clear();
}
