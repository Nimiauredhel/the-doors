/*
 * door_sensor.c
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#include "door_sensor.h"

#define TIMER_MHZ 1
#define SENSOR_BUFFER_LENGTH 32

#define PERIOD_TO_DIST_CM_FLOAT(period) (period / TIMER_MHZ / cm_to_us)
#define DIST_CM_FLOAT_TO_PERIOD(dist_cm_float) (dist_cm_float * TIMER_MHZ * cm_to_us)

static const float cm_to_us = 58.31f;
static const char *result_format = "[%02u] %.2fcm.(%luus)";

static volatile uint8_t sensor_buffer_idx = 0;
static volatile uint32_t sensor_buffer[SENSOR_BUFFER_LENGTH] = {0};

static volatile uint32_t capture_start_us = 0;

static volatile uint8_t capturing = 0;
static bool debug_enabled = false;

static void door_sensor_trigger(void)
{
	static const uint32_t trigger_end_us = 4000;

	if (capturing > 0) return;

	HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_SET);
	htim2.Instance->CNT = 0;
	while (htim2.Instance->CNT < trigger_end_us);
	HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_RESET);
}

static void door_sensor_record(uint32_t period_us)
{
	// capping this at uint16 value since we don't care about long distance, only short
	sensor_buffer[sensor_buffer_idx] = period_us;
	sensor_buffer_idx++;
	if (sensor_buffer_idx >= SENSOR_BUFFER_LENGTH) sensor_buffer_idx = 0;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
		// rising edge
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
		{
			// record echo start
			capturing++;
			capture_start_us = htim->Instance->CNT;
		}
		// falling edge
		else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		{
			// record echo width
			if (capturing > 0) capturing--;
			door_sensor_record(htim->Instance->CNT - capture_start_us);
			door_sensor_trigger();
		}
    }
}

void door_sensor_toggle_debug(void)
{
	debug_enabled = !debug_enabled;
}

void door_sensor_init(void)
{
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
	door_sensor_trigger();
}

void door_sensor_loop(void)
{
	static int8_t sample_idx = -1;
	static uint16_t sample_value;

	char result_buff[64] = {0};

	if (debug_enabled)
	{
		if (sample_idx != sensor_buffer_idx)
		{
			sample_idx = sensor_buffer_idx;
			sample_value = sensor_buffer[sample_idx];
			sprintf(result_buff, result_format, sample_idx, PERIOD_TO_DIST_CM_FLOAT(sample_value), sample_value);
			serial_print_line(result_buff, 0);
		}
	}
}

bool door_sensor_leq_cm(float min_cm)
{
	uint8_t idx;
	uint32_t min_period = DIST_CM_FLOAT_TO_PERIOD(min_cm);

	for (idx = 0; idx < SENSOR_BUFFER_LENGTH; idx++)
	{
		if (sensor_buffer[idx] <= min_period) return true;
	}

	return false;
}
