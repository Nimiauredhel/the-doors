/*
 * door_control.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_control.h"

static const int door_open_pulse = 800;
static const int door_close_pulse = 1600;

static bool door_closed = false;

static const char *redundant_msg = "Command Redundant.";
static const char *closing_msg = "Closing Door!";
static const char *opening_msg = "Opening Door!";

void door_control_init(void)
{
	door_closed = false;
	htim3.Instance->ARR = 20000-1;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	htim3.Instance->CCR1 = 800;
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
			htim3.Instance->CCR1 = door_close_pulse;
		}
		else
		{
			htim3.Instance->CCR1 = door_open_pulse;
		}

		door_closed = closed;
		return true;
	}
	else
	{
		return false;
	}
}
