/*
 * door_control.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef DOOR_CONTROL_H_
#define DOOR_CONTROL_H_

#include "main.h"
#include <stdbool.h>
#include <string.h>

extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim3;

void door_control_init(void);
void door_set_state(bool open);


#endif /* DOOR_CONTROL_H_ */
