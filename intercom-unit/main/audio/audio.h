#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <math.h>
#include "common.h"
#include "pitches.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AUDIO_PWM_PIN (13)
#define AUDIO_GND_PIN (-1)

void init_audio(void);
void set_audio(uint32_t pitch_period, float duty_percent);
void deinit_audio(void);
void stop_audio(void);
void audio_sfx_touch_down(void);
void audio_sfx_touch_up(void);
void audio_sfx_confirm(void);
void audio_sfx_cancel(void);
void audio_success_jingle(void);
void audio_failure_jingle(void);
void audio_still_open_reminder(void);
void audio_top_phase_jingle(void);

#endif
