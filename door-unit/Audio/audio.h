#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <math.h>

#include "main.h"

#include "pitches.h"
#include "uart_io.h"

extern TIM_HandleTypeDef htim9;

void init_audio(void);
void stop_audio(void);
void set_audio(uint32_t pitch_period, float duty_percent);

void audio_top_phase_jingle(void);
void audio_success_jingle(void);
void audio_failure_jingle(void);
void audio_sfx_touch_down(void);
void audio_sfx_touch_hover_in(void);
void audio_sfx_touch_hover_out(void);
void audio_sfx_touch_up(void);
void audio_click_sound(void);

#endif
