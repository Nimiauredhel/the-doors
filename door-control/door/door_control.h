/*
 * door_control.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef DOOR_CONTROL_H_
#define DOOR_CONTROL_H_

#include <stdbool.h>
#include <string.h>

#include "main.h"
#include "serial_io.h"

typedef enum DoorFlags
{
	DOOR_FLAG_NONE       = 0b00000000,
	DOOR_FLAG_CLOSED     = 0b00000001,
	DOOR_FLAG_TRANSITION = 0b00000010,
} DoorFlags_t;

extern TIM_HandleTypeDef htim3;

void door_control_init(void);
bool door_is_closed(void);
bool door_set_closed(bool closed);


#endif /* DOOR_CONTROL_H_ */
