#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f7/rcc.h>

#define LED_PORT (GPIOB)
#define LED_PIN  (GPIO14)

static void rcc_setup(void) {
  rcc_clock_setup_hsi(&rcc_3v3[RCC_CLOCK_3V3_72MHZ]);
}

static void gpio_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

static void delay_cycles(uint32_t cycles) {
  for (uint32_t i = 0; i < cycles; i++) {
    __asm__("nop");
  }
}

int main(void) {
  rcc_setup();
  gpio_setup();

  while (1) {
    gpio_toggle(LED_PORT, LED_PIN);
    delay_cycles(7200000 / 4);
  }

  // Never return
  return 0;
}
