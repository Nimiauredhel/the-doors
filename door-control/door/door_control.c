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

static const char *redundant_msg = "Command Redundant.\n\r";
static const char *closing_msg = "Closing Door!\n\r";
static const char *opening_msg = "Opening Door!\n\r";

void door_control_init(void)
{
	door_closed = false;
	htim3.Instance->ARR = 20000-1;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	htim3.Instance->CCR1 = 800;
}

void door_set_state(bool closed)
{
	if (closed != door_closed)
	{
		if (closed)
		{
			HAL_UART_Transmit(&huart3, (uint8_t *)closing_msg, strlen(closing_msg), 0xff);
			htim3.Instance->CCR1 = door_close_pulse;
		}
		else
		{
			HAL_UART_Transmit(&huart3, (uint8_t *)opening_msg, strlen(opening_msg), 0xff);
			htim3.Instance->CCR1 = door_open_pulse;
		}

		door_closed = closed;
	}
	else
	{
			HAL_UART_Transmit(&huart3, (uint8_t *)redundant_msg, strlen(redundant_msg), 0xff);
	}
}
