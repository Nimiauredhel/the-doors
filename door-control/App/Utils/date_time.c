/*
 * date_time.c
 *
 *  Created on: Apr 26, 2025
 *      Author: mickey
 */

/** @addtogroup Door_Unit_Utils
  * @{
  */

#include "date_time.h"

extern RTC_HandleTypeDef hrtc;

static RTC_TimeTypeDef time_now;
static RTC_DateTypeDef date_now;
static char output_buffer[64];

RTC_TimeTypeDef time_get()
{
    return time_now;
}

RTC_DateTypeDef date_get()
{
    return date_now;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	static RTC_AlarmTypeDef sAlarm;
	static uint8_t seconds_symbol = 0;
	static bool led_on = false;

	// update the global timestamp every alarm
	HAL_RTC_GetTime(hrtc, &time_now, FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &date_now, FORMAT_BIN);

	// increment the alarm so it keeps going
	HAL_RTC_GetAlarm(hrtc,&sAlarm,RTC_ALARM_A,FORMAT_BIN);

	if(sAlarm.AlarmTime.Seconds < 59)
	{
		sAlarm.AlarmTime.Seconds += 1;
	}
	else
	{
		sAlarm.AlarmTime.Seconds = 0;
	}

	while(HAL_RTC_SetAlarm_IT(hrtc, &sAlarm, FORMAT_BIN)!=HAL_OK)
	{
	}

	// indicate current 'seconds' count in binary with the onboard leds
	// TODO: check if the onboard LED pins are in conflict
	led_on = !led_on;
	seconds_symbol = led_on ? 1 + (sAlarm.AlarmTime.Seconds / 10) : 0x0;
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, 0x01 & seconds_symbol);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0x02 & seconds_symbol);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0x04 & seconds_symbol);
}

void date_time_alarm_reset()
{
	static RTC_AlarmTypeDef sAlarm;

	// update the global timestamp
	HAL_RTC_GetTime(&hrtc, &time_now, FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date_now, FORMAT_BIN);

	// set the alarm so it keeps going
	HAL_RTC_GetAlarm(&hrtc,&sAlarm,RTC_ALARM_A,FORMAT_BIN);

	if (time_now.Seconds >= 59) sAlarm.AlarmTime.Seconds = 0;
	else sAlarm.AlarmTime.Seconds = time_now.Seconds + 1;

	while(HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, FORMAT_BIN)!=HAL_OK)
	{
	}
}

void date_time_print()
{
	sprintf(output_buffer, "\n\rThe current date is: %02u/%02u/%02u.", date_now.Date, date_now.Month, date_now.Year);
    serial_print_line(output_buffer, 0);
	sprintf(output_buffer, "The current time is: %02u:%02u:%02u.", time_now.Hours, time_now.Minutes, time_now.Seconds);
    serial_print_line(output_buffer, 0);
}

void date_time_set()
{
	RTC_DateTypeDef sDate = {0};
	RTC_TimeTypeDef sTime = {0};
	int8_t num = 0;
	char input_buffer[2] = {0};

	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	serial_print_line("Let's set the date.\n\rAll values are two digits including leading zero.", 0);

	// receive and validate year input value
	// TODO: validate numeric input in this and following prompts
	do
	{
		serial_print("Enter year (last two digits, STM is not Y2K prepared): ", 0);
		serial_scan(input_buffer, 2);
		serial_print_line(NULL, 0);
		num = atoi(input_buffer);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	while (num < 0 || num > 99);

	sDate.Year = num;
	vTaskDelay(pdMS_TO_TICKS(100));

	// receive and validate month input value
	do
	{
		serial_print("Enter month: ", 0);
		serial_scan(input_buffer, 2);
        serial_print_line(NULL, 0);
		num = atoi(input_buffer);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	while (num < 1 || num > 12);

	sDate.Month = num;
	vTaskDelay(pdMS_TO_TICKS(100));

	do
	{
		serial_print("Enter day of the month: ", 0);
		serial_scan(input_buffer, 2);
		serial_print_line(NULL, 0);
		num = atoi(input_buffer);
	} while (num < 1 || num > 31);

	sDate.Date = num;
	vTaskDelay(pdMS_TO_TICKS(100));

	serial_print_line("Let's set the time.\n\rAll values are two digits including leading zero.", 0);

	do
	{
		serial_print("Enter hour: ", 0);
		serial_scan(input_buffer, 2);
		serial_print_line(NULL, 0);
		num = atoi(input_buffer);
		vTaskDelay(pdMS_TO_TICKS(100));
	} while (num < 0 || num > 23);

	sTime.Hours = num;
	vTaskDelay(pdMS_TO_TICKS(100));

	do
	{
		serial_print("Enter minutes: ", 0);
		serial_scan(input_buffer, 2);
		serial_print_line(NULL, 0);
		num = atoi(input_buffer);
		vTaskDelay(pdMS_TO_TICKS(100));
	} while (num < 0 || num > 59);

	sTime.Minutes = num;
	vTaskDelay(pdMS_TO_TICKS(100));

	do
	{
		serial_print("Enter seconds: ", 0);
		serial_scan(input_buffer, 2);
		serial_print_line(NULL, 0);
		num = atoi(input_buffer);
		vTaskDelay(pdMS_TO_TICKS(100));
	} while (num < 0 || num > 59);

	sTime.Seconds = num;
	vTaskDelay(pdMS_TO_TICKS(100));

	serial_print_line("Input accepted, setting time/date...", 0);

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		serial_print_line("Error setting the date.", 0);
		//Error_Handler();
	}

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		serial_print_line("Error setting the time.", 0);
		//Error_Handler();
	}

	date_time_alarm_reset();
	serial_print_line("Let's wait a second...", 0);
	vTaskDelay(pdMS_TO_TICKS(1000));
	date_time_print();
	vTaskDelay(pdMS_TO_TICKS(100));
}

/**
  * @}
  */
