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
	HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
}

void audio_sfx_touch_down(void)
{
	set_audio(E3, 0.2f);
	vTaskDelay(pdMS_TO_TICKS(50));
	stop_audio();
}

void audio_sfx_touch_up(void)
{
	set_audio(F4, 0.2f);
	vTaskDelay(pdMS_TO_TICKS(50));
	stop_audio();
}

void audio_click_sound(void)
{
	float mod = 1.0f;
	uint32_t base = A3;

	for (int i = 0; i < 10; i++)
	{
		set_audio(base * mod, 0.25f);
		mod -= 0.05f;
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	stop_audio();
}

void audio_success_jingle(void)
{
	set_audio(C3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(250));
	set_audio(D3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(250));
	set_audio(E3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(250));
	set_audio(F3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(250));
	stop_audio();
}

void audio_failure_jingle(void)
{
	set_audio(C3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(450));
	set_audio(D3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(150));
	set_audio(Eb3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(300));
	set_audio(C3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(300));
	stop_audio();
}

void audio_still_open_reminder(void)
{
	set_audio(Eb4, 0.1f);
	vTaskDelay(pdMS_TO_TICKS(125));
	stop_audio();
	vTaskDelay(pdMS_TO_TICKS(125));
	set_audio(C4, 0.1f);
	vTaskDelay(pdMS_TO_TICKS(125));
	stop_audio();
	vTaskDelay(pdMS_TO_TICKS(125));
	set_audio(Eb4, 0.1f);
	vTaskDelay(pdMS_TO_TICKS(125));
	stop_audio();
	vTaskDelay(pdMS_TO_TICKS(125));
	set_audio(C4, 0.1f);
	vTaskDelay(pdMS_TO_TICKS(125));
	stop_audio();
	vTaskDelay(pdMS_TO_TICKS(125));
}

void audio_top_phase_jingle(void)
{
	set_audio(Gb3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(G3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(Gb3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(G3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(Gb3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(E3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(D3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(Gb3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(75));
	set_audio(G3, 0.25f);
	vTaskDelay(pdMS_TO_TICKS(200));
	stop_audio();
}
