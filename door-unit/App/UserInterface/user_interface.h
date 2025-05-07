/*
 * door_interface.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_

#include <door_date_time.h>
#include <string.h>
#include <stdbool.h>
#include <user_auth.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "gfx.h"

#include "display.h"
#include "uart_io.h"
#include "door_control.h"
#include "hub_comms.h"

typedef enum InterfacePhase
{
	IPHASE_NONE = 0,
	IPHASE_TOP = 1,
	IPHASE_CHECKPW = 2,
	IPHASE_SETPW = 3,
	IPHASE_OPEN = 4,
	IPHASE_CLOSE = 5,
	IPHASE_SETTIME = 6,
} InterfacePhase_t;

void interface_init(void);
void interface_loop(void);

#endif /* USER_INTERFACE_H_ */
