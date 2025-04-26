/*
 * date_time.h
 *
 *  Created on: Nov 22, 2024
 *      Author: mickey
 */

/** @addtogroup Door_Unit_Utils
  * @{
  */

#ifndef DATE_TIME_H_
#define DATE_TIME_H_

#include <stdlib.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include "uart_io.h"

RTC_TimeTypeDef time_get();
RTC_DateTypeDef date_get();
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);
void date_time_alarm_reset();
void date_time_print();
void date_time_set();

#endif /* DATE_TIME_H_ */

/** 
  * @}
  */
