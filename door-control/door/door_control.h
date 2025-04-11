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

extern TIM_HandleTypeDef htim3;

void door_control_init(void);
bool door_get_state(void);
bool door_set_state(bool open);


#endif /* DOOR_CONTROL_H_ */
