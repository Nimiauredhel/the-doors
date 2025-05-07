#include "audio.h"

void init_audio(void)
{
    serial_print_line("Initializing audio.", 0);
	HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_1);
	htim9.Instance->CCR1 = 0;
	htim9.Instance->ARR = 0;
}

void stop_audio(void)
{
	HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_1);
	htim9.Instance->CCR1 = 0;
	htim9.Instance->ARR = 0;
}

void set_audio(uint32_t pitch_period, float duty_percent)
{
	htim9.Instance->ARR = pitch_period;
	htim9.Instance->CCR1 = pitch_period * duty_percent;
}

void deinit_audio(void)
{
}
