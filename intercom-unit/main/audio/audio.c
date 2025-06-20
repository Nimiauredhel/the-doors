#include "audio.h"

static const ledc_timer_config_t audio_timer_config =
{
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 440,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .clk_cfg = LEDC_USE_REF_TICK,
};

static const ledc_channel_config_t audio_channel_config =
{
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty = 0,
    .channel = LEDC_CHANNEL_0,
    .gpio_num = AUDIO_PWM_PIN,
    .timer_sel = LEDC_TIMER_0,
};

void init_audio(void)
{
    printf("Initializing LEDC for audio.\n");

    if (AUDIO_GND_PIN >= 0)
    {
        gpio_set_pull_mode(AUDIO_GND_PIN, GPIO_PULLDOWN_ONLY);
        gpio_set_level(AUDIO_GND_PIN, 0);
    }

    ESP_ERROR_CHECK(ledc_timer_config(&audio_timer_config));
    ESP_ERROR_CHECK(ledc_channel_config(&audio_channel_config));
}

void set_audio(uint32_t pitch_period, float duty_percent)
{
    uint32_t duty_resolution = 8;
    uint32_t duty_max = 255;

    if (duty_percent > 0.0f)
    {
        ESP_ERROR_CHECK(ledc_timer_set(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, pitch_period, duty_resolution, LEDC_REF_TICK));
    }

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_max * duty_percent));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

void stop_audio(void)
{
	set_audio(0, 0);
}

void deinit_audio(void)
{
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

void audio_sfx_confirm(void)
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

void audio_sfx_cancel(void)
{
	float mod = 1.5f;
	uint32_t base = A3;

	for (int i = 0; i < 10; i++)
	{
		set_audio(base * mod, 0.25f);
		mod += 0.05f;
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
