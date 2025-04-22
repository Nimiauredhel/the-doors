/*
 * door_sensor.c
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#include "door_sensor.h"

#define TIMER_MHZ 1
#define SENSOR_BUFFER_LENGTH 8

#define PERIOD_TO_DIST_CM_FLOAT(period) (period / TIMER_MHZ / cm_to_us)
#define DIST_CM_FLOAT_TO_PERIOD(dist_cm_float) (dist_cm_float * TIMER_MHZ * cm_to_us)

static const float cm_to_us = 58.31f;
static const char *result_format = "%.2fcm.  (%luus)";

static volatile uint8_t sensor_buffer_idx = 0;
static volatile uint16_t sensor_buffer[SENSOR_BUFFER_LENGTH] = {0};

static bool debug_enabled = false;

static void door_sensor_trigger(void)
{
	HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_SET);
	HAL_TIM_Base_Start_IT(&htim5);
}

static void door_sensor_record(uint32_t period_us)
{
	// capping this at uint16 value since we don't care about long distance, only short
	if (period_us > UINT16_MAX) period_us = UINT16_MAX;
	sensor_buffer[sensor_buffer_idx] = period_us;
	sensor_buffer_idx++;
	if (sensor_buffer_idx >= SENSOR_BUFFER_LENGTH) sensor_buffer_idx = 0;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
		volatile GPIO_PinState state = HAL_GPIO_ReadPin(DOOR_SENSOR_ECHO_GPIO_Port, DOOR_SENSOR_ECHO_Pin);

		if (state == GPIO_PIN_SET)
		{
			// start timer
			htim->Instance->CNT &= 0x0;
			HAL_TIM_Base_Start(&htim2);
		}
		else
		{
			// stop timer
			HAL_TIM_Base_Stop(&htim2);
			door_sensor_record(htim->Instance->CNT);
			htim->Instance->CNT &= 0x0;
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
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
	door_sensor_trigger();
}

void door_sensor_loop(void)
{
	char result_buff[64] = {0};
	uint32_t result;

	if (debug_enabled)
	{
		result = sensor_buffer[sensor_buffer_idx];
		sprintf(result_buff, result_format, PERIOD_TO_DIST_CM_FLOAT(result), result);
		serial_print_line(result_buff, 0);
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
