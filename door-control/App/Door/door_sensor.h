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
extern TIM_HandleTypeDef htim6;

void door_sensor_init(void);
void door_sensor_loop(void);
void door_sensor_toggle_debug(void);
bool door_sensor_leq_cm(float min_cm);

#endif /* DOOR_SENSOR_H_ */
