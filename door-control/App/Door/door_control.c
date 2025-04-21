/*
 * door_control.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_control.h"

static const int door_open_angle = 5;
static const int door_close_angle = 95;

static bool initialized = false;
static DoorFlags_t door_state_flags = DOOR_FLAG_NONE;
static int16_t servo_last_angle;
static volatile uint16_t door_open_duration_seconds = 0;

static void set_door_indicator_led(float open_percent)
{
	if (open_percent < 0.0f)
	{
		htim1.Instance->CCR3 = 0;
		htim1.Instance->CCR2 = 0;
	}
	else
	{
		htim1.Instance->CCR3 = htim1.Instance->ARR * (1.0f - open_percent);
		htim1.Instance->CCR2 = htim1.Instance->ARR * open_percent;
	}
}

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
	servo_last_angle = angle;

}

static void servo_set_angle_gradual(int16_t target_angle, uint16_t step_size, uint16_t step_delay, float min_dist)
{
	if (step_size == 0) return;
	bool light_on = false;

	uint16_t step = (0 > target_angle - servo_last_angle) ? -step_size : step_size;

	if (step_size < 1) step_size = 1;
	//if (step_delay < step_size) step_delay = step_size;

	bool blocked = false;

	while(abs(target_angle - servo_last_angle) > step_size)
	{
		vTaskDelay(pdMS_TO_TICKS(step_delay));
		light_on = !light_on;
		set_door_indicator_led(light_on ? (float)(servo_last_angle-door_open_angle)/(float)(door_close_angle-door_open_angle) : -1.0f);
		if (min_dist > 0.0f)
		{
			door_sensor_read();
			if (door_sensor_get_last_read_cm() <= min_dist)
			{
				if (!blocked)
				{
					blocked = true;
					event_log_append(PACKET_REPORT_DOOR_BLOCKED, 0);
				}

				continue;
			}
			else blocked = false;
		}
		servo_set_angle(servo_last_angle + step);
	}

	servo_set_angle(target_angle);
}

bool door_control_is_init(void)
{
	return initialized;
}

void door_control_init(void)
{
	serial_print_line("Initializing Door Control.", 0);
	htim3.Instance->ARR = 20000-1;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	htim1.Instance->ARR = 65535;
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

	door_sensor_init();

	door_set_closed(true);
	initialized = true;
	vTaskDelay(pdMS_TO_TICKS(500));
	serial_print_line("Door Control Initialized.", 0);

}

void door_control_loop(void)
{
	vTaskDelay(1);

	static uint16_t last_timer_notification = 0;

	char msg_buff[64];

	if (door_open_duration_seconds >= 15)
	{
		if (door_open_duration_seconds != last_timer_notification
		&& door_open_duration_seconds % 10 == 5)
		{
			last_timer_notification = door_open_duration_seconds;
			sprintf(msg_buff, "The door has been open for %u seconds.", door_open_duration_seconds);
			serial_print_line(msg_buff, 0);
			door_set_closed(true);
		}
	}
	else
	{
		last_timer_notification = 0;
	}
}

void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
	static uint8_t divisor = 0;

	if (hlptim->Instance == LPTIM1)
	{
		divisor++;

		if (divisor >= 10)
		{
			divisor = 0;
			door_timer_callback();
		}
	}
}

void door_timer_callback(void)
{
	if (!(door_state_flags & DOOR_FLAG_CLOSED) || door_state_flags & DOOR_FLAG_TRANSITION)
	{
		door_open_duration_seconds++;
	}
	else door_open_duration_seconds = 0;
}

bool door_is_closed(void)
{
	return door_state_flags & DOOR_FLAG_CLOSED;
}

bool door_set_closed(bool closed)
{
	if (closed == (door_state_flags & DOOR_FLAG_CLOSED)) return false;

	door_state_flags ^= DOOR_FLAG_CLOSED;

	if (initialized)
	{
		serial_print_line("Door Changing State...", 0);
		door_state_flags |= DOOR_FLAG_TRANSITION;
		if (!closed) event_log_append(PACKET_REPORT_DOOR_OPENED, 0);
		servo_set_angle_gradual(closed ? door_close_angle : door_open_angle,
		1, closed ? 1 : 30, closed ? 4.5f : 0.0f);
		door_state_flags &= ~DOOR_FLAG_TRANSITION;
		if (closed) event_log_append(PACKET_REPORT_DOOR_CLOSED, 0);
		serial_print_line("Door State Changed.", 0);
	}
	else
	{
		serial_print_line("Initializing Door State.", 0);
		servo_set_angle(closed ? door_close_angle : door_open_angle);
		if (closed) event_log_append(PACKET_REPORT_DOOR_CLOSED, 0);
		vTaskDelay(pdMS_TO_TICKS(500));
	}

	set_door_indicator_led(closed ? 1.0f : 0.0f);

	return true;
}
