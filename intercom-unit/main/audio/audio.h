#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <math.h>
#include "common.h"
#include "pitches.h"
#include "driver/ledc.h"

void init_audio(void);
void set_audio(uint32_t pitch_period, float duty_percent);
void deinit_audio(void);

#endif
