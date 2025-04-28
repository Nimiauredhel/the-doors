/*
 * door_date_time.h
 *
 *  Created on: Apr 26, 2025
 *      Author: mickey
 */

/** @addtogroup Door_Unit_Utils
  * @{
  */

#ifndef DOOR_DATE_TIME_H_
#define DOOR_DATE_TIME_H_

#include <stdlib.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include "uart_io.h"
#include "event_log.h"

RTC_TimeTypeDef time_get();
RTC_DateTypeDef date_get();
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);
void date_time_alarm_reset();
void date_time_print();
void date_time_set_interactive();
void date_time_set_from_packet(uint16_t date, uint32_t time);

#endif /* DOOR_DATE_TIME_H_ */

/** 
  * @}
  */
