/*
 * door_sensor.h
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#ifndef DOOR_SENSOR_H_
#define DOOR_SENSOR_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "event_log.h"
#include "uart_io.h"

extern TIM_HandleTypeDef htim2;

void door_sensor_init(void);
void door_sensor_read(void);
float door_sensor_get_last_read_cm(void);
void door_sensor_toggle_debug(void);

#endif /* DOOR_SENSOR_H_ */
