/*
 * door_sensor.c
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#include "door_sensor.h"

#define TIMER_MHZ (1)
#define SENSOR_BUFFER_LENGTH (16)

#define PERIOD_TO_DIST_CM_FLOAT(period) (period / TIMER_MHZ / cm_to_us)
#define DIST_CM_FLOAT_TO_PERIOD(dist_cm_float) (dist_cm_float * TIMER_MHZ * cm_to_us)

const uint16_t sensor_max_echo_us = 32768;
const uint16_t sensor_reboot_delay_us = UINT16_MAX;

static const float cm_to_us = 58.31f;
static const char *result_format = "[%02u] %.2fcm.(%luus)";

static volatile DoorSensorState_t sensor_state = SENSOR_STATE_OFF;
static volatile uint8_t sensor_buffer_idx = 0;
static volatile uint16_t sensor_buffer[SENSOR_BUFFER_LENGTH] = {0};
static volatile uint16_t capture_start_us = 0;

static bool debug_enabled = false;

static void door_sensor_record(uint32_t period_us)
{
	// capping this at max value since we don't care about long distance, only short
	// reading it as zero since it's often an erroneous value under min distance
	if (period_us > sensor_max_echo_us) period_us = 0;
	sensor_buffer[sensor_buffer_idx] = period_us;
	sensor_buffer_idx++;
	if (sensor_buffer_idx >= SENSOR_BUFFER_LENGTH) sensor_buffer_idx = 0;
}

static void door_sensor_print_debug(void)
{
	static int8_t sample_idx = -1;
	static uint16_t sample_value;

	char result_buff[128] = {0};

	if (sample_idx != sensor_buffer_idx)
	{
		sample_idx = sensor_buffer_idx;
		sample_value = sensor_buffer[sample_idx];
		snprintf(result_buff, sizeof(result_buff), result_format, sample_idx, PERIOD_TO_DIST_CM_FLOAT(sample_value), sample_value);
		serial_print_line(result_buff, 0);
	}
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
		// rising edge
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
		{
			// record echo start
			if (sensor_state == SENSOR_STATE_TRIGGER)
			{
				HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_RESET);
				sensor_state = SENSOR_STATE_CAPTURE;
				capture_start_us = __HAL_TIM_GET_COUNTER(htim);
			}
		}
		// falling edge
		else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		{
			// record echo width
			if (sensor_state == SENSOR_STATE_CAPTURE)
			{
				if(__HAL_TIM_GET_COUNTER(htim) <= capture_start_us)
				{
					door_sensor_record(0);
				}
				else
				{
					door_sensor_record(__HAL_TIM_GET_COUNTER(htim) - capture_start_us);
				}

				sensor_state = SENSOR_STATE_IDLE;
			}
		}
    }
}

void door_sensor_toggle_debug(void)
{
	debug_enabled = !debug_enabled;
}

void door_sensor_enable(bool from_reboot)
{
	if (sensor_state != SENSOR_STATE_OFF
		&& !(from_reboot && sensor_state == SENSOR_STATE_REBOOTING))
		return;

	__HAL_TIM_SET_AUTORELOAD(&htim2, sensor_max_echo_us);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
	sensor_state = SENSOR_STATE_IDLE;
}

void door_sensor_disable(void)
{
	if (sensor_state == SENSOR_STATE_OFF) return;

	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_4);
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_3);
	sensor_state = SENSOR_STATE_OFF;
}

void door_sensor_reboot(void)
{
	if (sensor_state == SENSOR_STATE_REBOOTING) return;

	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_4);
	HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_3);
	__HAL_TIM_SET_AUTORELOAD(&htim2, sensor_reboot_delay_us);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
	sensor_state = SENSOR_STATE_REBOOTING;
	HAL_TIM_Base_Start_IT(&htim2);
}

void door_sensor_loop(void)
{
	vTaskDelay(pdMS_TO_TICKS(1));

	if (sensor_state == SENSOR_STATE_IDLE)
	{
		door_sensor_trigger();
	}
	else
	{
		vTaskDelay(50);
	}

	if (debug_enabled)
	{
		door_sensor_print_debug();
	}
}

void door_sensor_trigger(void)
{
	static const uint16_t trig_length_us = 10;
	static const uint16_t trig_cooldown_us = sensor_max_echo_us;

	if (__HAL_TIM_GET_COUNTER(&htim2) < trig_cooldown_us) return;

	__HAL_TIM_SET_COUNTER(&htim2, 0);
	sensor_state = SENSOR_STATE_TRIGGER;
	HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_SET);
	while (__HAL_TIM_GET_COUNTER(&htim2) < trig_length_us);
	HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_RESET);
}


bool door_sensor_leq_cm(float min_cm)
{
	uint8_t idx;
	uint16_t min_period = DIST_CM_FLOAT_TO_PERIOD(min_cm);

	if (sensor_state == SENSOR_STATE_OFF) return true;

	for (idx = 0; idx < SENSOR_BUFFER_LENGTH; idx++)
	{
		if (sensor_buffer[idx] <= min_period) return true;
	}

	return false;
}

DoorSensorState_t door_sensor_get_state(void)
{
	return sensor_state;
}
