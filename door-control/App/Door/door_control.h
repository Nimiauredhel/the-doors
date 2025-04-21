/*
 * door_control.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef DOOR_CONTROL_H_
#define DOOR_CONTROL_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "door_sensor.h"
#include "event_log.h"
#include "uart_io.h"

typedef enum DoorFlags
{
	DOOR_FLAG_NONE       = 0b00000000,
	DOOR_FLAG_CLOSED     = 0b00000001,
	DOOR_FLAG_TRANSITION = 0b00000010,
} DoorFlags_t;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;

bool door_control_is_init(void);
void door_control_init(void);
void door_control_loop(void);
void door_timer_callback(void);
bool door_is_closed(void);
bool door_set_closed(bool closed);


#endif /* DOOR_CONTROL_H_ */
