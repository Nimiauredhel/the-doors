/*
 * door_control.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_control.h"

static const int door_open_angle = 10;
static const int door_close_angle = 90;

static bool door_closed = false;

static void servo_set_angle(int16_t angle)
{
	static const int16_t min_angle = 0;
	static const int16_t max_angle = 180;
	static const int16_t min_pulse_width = 575; // 0.575ms pulse width at a 1MHz clock
	static const int16_t max_pulse_width = 2500; // 2.5ms pulse width

	if (angle < min_angle) angle = min_angle;
	if (angle > max_angle) angle = max_angle;

	uint32_t pulse = ((angle * (max_pulse_width - min_pulse_width)) / max_angle) + min_pulse_width;
	htim3.Instance->CCR1 = pulse;
}

void door_control_init(void)
{
	serial_print_line("Initializing Door Control.", 0);
	door_closed = false;
	htim3.Instance->ARR = 20000-1;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	door_set_state(true);
	HAL_Delay(500);
	door_set_state(false);
	HAL_Delay(500);
	door_set_state(true);
	HAL_Delay(1000);
	serial_print_line("Door Control Initialized.", 0);
}

bool door_get_state(void)
{
	return door_closed;
}

bool door_set_state(bool closed)
{
	if (closed != door_closed)
	{
		if (closed)
		{
			servo_set_angle(door_close_angle);
		}
		else
		{
			servo_set_angle(door_open_angle);
		}

		door_closed = closed;
		return true;
	}
	else
	{
		return false;
	}
}
