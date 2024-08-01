#include "timer.h"
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

#define TIMER TIM12

#define PRESCALER 217
#define ARR_VALUE 1000

void timer_setup(void)
{
    rcc_periph_clock_enable(RCC_TIM12);
    timer_set_mode(TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_oc_mode(TIMER, TIM_OC1, TIM_OCM_PWM1);
    timer_enable_counter(TIMER);
    timer_enable_oc_output(TIMER, TIM_OC1);
    timer_set_prescaler(TIMER, PRESCALER - 1);
    timer_set_period(TIMER, ARR_VALUE - 1);
}

void timer_pwm_set_duty_cycle(float duty_cycle)
{
    const float raw_value = (float)ARR_VALUE * (duty_cycle / 100.0f);
    timer_set_oc_value(TIMER, TIM_OC1, (uint32_t)raw_value);
}
