/*
 * door_date_time.c
 *
 *  Created on: Apr 26, 2025
 *      Author: mickey
 */

/** @addtogroup Door_Unit_Utils
  * @{
  */

#include "door_date_time.h"

extern RTC_HandleTypeDef hrtc;

static volatile RTC_TimeTypeDef time_now;
static volatile RTC_DateTypeDef date_now;
static char output_buffer[64];

volatile RTC_TimeTypeDef time_get()
{
    return time_now;
}

volatile RTC_DateTypeDef date_get()
{
    return date_now;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	static RTC_AlarmTypeDef sAlarm;
	static uint8_t seconds_symbol = 0;
	static bool led_on = false;

	// update the global timestamp every alarm
	HAL_RTC_GetTime(hrtc, (RTC_TimeTypeDef *)&time_now, FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, (RTC_DateTypeDef *)&date_now, FORMAT_BIN);

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
	HAL_RTC_GetTime(&hrtc, (RTC_TimeTypeDef *)&time_now, FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, (RTC_DateTypeDef *)&date_now, FORMAT_BIN);

	// set the alarm so it keeps going
	HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, FORMAT_BIN);

	if (time_now.Seconds >= 59) sAlarm.AlarmTime.Seconds = 0;
	else sAlarm.AlarmTime.Seconds = time_now.Seconds + 1;

	while(HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, FORMAT_BIN) != HAL_OK)
	{
	}
}

void date_time_get_time_str_hhmm(char *buf)
{
	sprintf(buf, "%02u:%02u", time_now.Hours, time_now.Minutes);
}

void date_time_get_time_str(char *buf)
{
	sprintf(buf, "%02u:%02u:%02u", time_now.Hours, time_now.Minutes, time_now.Seconds);
}

void date_time_get_date_str(char *buf)
{
	sprintf(buf, "%02u/%02u/%02u", date_now.Date, date_now.Month, date_now.Year);
}

void date_time_get_str(char *buf)
{
	sprintf(buf, "%02u:%02u:%02u %02u/%02u/%02u", time_now.Hours, time_now.Minutes, time_now.Seconds, date_now.Date, date_now.Month, date_now.Year);
}

void date_time_print()
{
	sprintf(output_buffer, "\n\rThe current date is: %02u/%02u/%02u.", date_now.Date, date_now.Month, date_now.Year);
    serial_print_line(output_buffer, 0);
	sprintf(output_buffer, "The current time is: %02u:%02u:%02u.", time_now.Hours, time_now.Minutes, time_now.Seconds);
    serial_print_line(output_buffer, 0);
}

void date_time_set_from_packet(uint16_t date, uint32_t time)
{
	RTC_DateTypeDef sDate = {0};
	RTC_TimeTypeDef sTime = {0};

	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	sDate.Year = packet_decode_year(date);
	sDate.Month = packet_decode_month(date);
	sDate.Date = packet_decode_day(date);
	sTime.Hours = packet_decode_hour(time);
	sTime.Minutes = packet_decode_minutes(time);
	sTime.Seconds = packet_decode_seconds(time);

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
	event_log_append(PACKET_CAT_REPORT, PACKET_REPORT_TIME_SET,
			packet_encode_date(sDate.Year, sDate.Month, sDate.Date),
			packet_encode_time(sTime.Hours, sTime.Minutes, sTime.Seconds));
	date_time_print();
}

/**
  * @}
  */
