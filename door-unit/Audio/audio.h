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

#endif
