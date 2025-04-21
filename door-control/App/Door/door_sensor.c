/*
 * door_sensor.c
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#include "door_sensor.h"

#define TIMER_MHZ 72

static const uint8_t trig_length_ms = 8;
static const float cm_to_us = 58.2f;
static const char *result_format = "%.2fcm.  (%luus)";

static volatile bool sensor_busy = false;
static volatile uint32_t sensor_counter = 0;

static int64_t last_read_us = 0;
static float last_read_float_cm = 0;

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
			sensor_counter = htim->Instance->CNT;
			sensor_busy = false;
			htim->Instance->CNT &= 0x0;
		}
    }
}

static void door_sensor_process(int64_t result)
{
	char result_buff[64] = {0};

	last_read_float_cm = result / TIMER_MHZ / cm_to_us;

	sprintf(result_buff, result_format, last_read_float_cm, result);
	serial_print_line(result_buff, 0);
}

void door_sensor_read(void)
{
	static const uint8_t sample_count = 3;
	int64_t resultsum = 0;

	for (int i = 0; i < sample_count; i++)
	{
		sensor_counter = 0;
		sensor_busy = true;

		HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_SET);
		vTaskDelay(pdMS_TO_TICKS(trig_length_ms));
		HAL_GPIO_WritePin(DOOR_SENSOR_TRIG_GPIO_Port, DOOR_SENSOR_TRIG_Pin, GPIO_PIN_RESET);

		while (sensor_busy)
		{
			vTaskDelay(pdMS_TO_TICKS(1));
		}

		resultsum += sensor_counter;
	}

	last_read_us = resultsum / sample_count;

	door_sensor_process(last_read_us);
}

void door_sensor_init(void)
{
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
}

float door_sensor_get_last_read_cm(void)
{
	return last_read_float_cm;
}
