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

typedef enum DoorSensorState
{
	SENSOR_STATE_NONE = 0,
	SENSOR_STATE_OFF = 1,
	SENSOR_STATE_IDLE = 2,
	SENSOR_STATE_TRIGGER = 3,
	SENSOR_STATE_CAPTURE = 4,
	SENSOR_STATE_REBOOTING = 5,
} DoorSensorState_t;

extern const uint16_t sensor_max_echo_us;

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim6;

void door_sensor_enable(bool from_reboot);
void door_sensor_disable(void);
void door_sensor_reboot(void);
void door_sensor_loop(void);
void door_sensor_trigger(void);
bool door_sensor_leq_cm(float min_cm);
void door_sensor_toggle_debug(void);
DoorSensorState_t door_sensor_get_state(void);

#endif /* DOOR_SENSOR_H_ */
