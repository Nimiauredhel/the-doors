/*
 * door_control.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_control.h"

static const int door_open_angle = 0;
static const int door_close_angle = 90;

static bool initialized = false;
static DoorFlags_t door_state_flags = DOOR_FLAG_NONE;
static int16_t servo_last_angle;
static volatile uint32_t door_open_duration_seconds = 0;

static void servo_set_angle(int16_t angle, uint16_t time_limit_ms)
{
	static const int16_t min_angle = 0;
	static const int16_t max_angle = 180;
	static const int16_t min_pulse_width = 575; // 0.575ms pulse width at a 1MHz clock
	static const int16_t max_pulse_width = 2500; // 2.5ms pulse width

	if (angle < min_angle) angle = min_angle;
	if (angle > max_angle) angle = max_angle;

	uint32_t pulse = ((angle * (max_pulse_width - min_pulse_width)) / max_angle) + min_pulse_width;
	htim3.Instance->CCR1 = pulse;

	if (time_limit_ms > 0)
	{
		vTaskDelay(pdMS_TO_TICKS(time_limit_ms));
		htim3.Instance->CCR1 = 0;
	}
	else
	{
		servo_last_angle = angle;
	}

}

static void servo_set_angle_gradual(int16_t target_angle, uint16_t step_size, uint16_t step_delay, float min_dist)
{
	static const int16_t blocked_report_gap_ms = 10000;

	if (step_size == 0) return;

	int16_t blocked_report_timer_ms = 0;
	int16_t idx = 0;
	uint16_t steps[128] = {0};

	if (step_size < 1) step_size = 1;
	//if (step_delay < step_size) step_delay = step_size;

	uint16_t step_count = abs(target_angle - servo_last_angle) / step_size;
	int16_t step = (0 > target_angle - servo_last_angle) ? -step_size : step_size;

	if (step_count > 128) step_count = 128;
	steps[0] = servo_last_angle;

	for (idx = 1; idx < step_count; idx++)
	{
		steps[idx] = steps[idx-1] + step;
	}

	if (min_dist > 0.0f)
	{
		door_sensor_enable(false);
	}

	for (idx = 1; idx < step_count;)
	{
		if (min_dist > 0.0f && door_sensor_leq_cm(min_dist))
		{

			if (blocked_report_timer_ms <= 0)
			{
				event_log_append(PACKET_REPORT_DOOR_BLOCKED, 0, door_open_duration_seconds);
				blocked_report_timer_ms = blocked_report_gap_ms;
			}
			else
			{
				blocked_report_timer_ms -= step_delay/2;
			}

			if (idx > 0)
			{
				idx--;
			}

			vTaskDelay(pdMS_TO_TICKS(step_delay/2));
		}
		else
		{
			idx++;
			blocked_report_timer_ms = 0;
			vTaskDelay(pdMS_TO_TICKS(step_delay));
		}

		servo_set_angle(steps[idx], 0);
	}

	servo_set_angle(target_angle, 0);
	door_sensor_disable();
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

	door_set_closed(true);
	initialized = true;
	serial_print_line("Door Control Initialized.", 0);
	vTaskDelay(pdMS_TO_TICKS(500));
	door_sensor_enable(false);

}

void door_control_loop(void)
{
	static uint16_t last_timer_notification = 0;

	char msg_buff[64];

	if (door_open_duration_seconds >= 15)
	{
		if (door_open_duration_seconds != last_timer_notification
		&& door_open_duration_seconds % 10 == 5)
		{
			last_timer_notification = door_open_duration_seconds;
			sprintf(msg_buff, "The door has been open for %lu seconds.", door_open_duration_seconds);
			serial_print_line(msg_buff, 0);
			door_set_closed(true);
		}
	}
	else
	{
		last_timer_notification = 0;
	}

	vTaskDelay(pdMS_TO_TICKS(
		door_is_closed() ? 250 : 10));
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
		if (!closed) event_log_append_minimal(PACKET_REPORT_DOOR_OPENED);
		servo_set_angle_gradual(closed ? door_close_angle + 10 : door_open_angle,
		1, closed ? 20 : 10, closed ? 5.0f : 0.0f);
		if (closed) servo_set_angle(door_close_angle, 0);
		door_state_flags &= ~DOOR_FLAG_TRANSITION;
		if (closed) event_log_append_minimal(PACKET_REPORT_DOOR_CLOSED);
		serial_print_line("Door State Changed.", 0);
	}
	else
	{
		serial_print_line("Initializing Door State.", 0);

		for (int i = 0; i < 20; i++)
		{
			servo_set_angle(closed ? door_close_angle : door_open_angle, 5);
			vTaskDelay(pdMS_TO_TICKS(45));
		}

		servo_set_angle(closed ? door_close_angle : door_open_angle, 0);
		if (closed) event_log_append_minimal(PACKET_REPORT_DOOR_CLOSED);
		vTaskDelay(pdMS_TO_TICKS(50));
	}

	return true;
}
