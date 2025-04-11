/*
 * door_interface.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef DOOR_INTERFACE_H_
#define DOOR_INTERFACE_H_

#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "serial_io.h"
#include "door_control.h"
#include "door_auth.h"

typedef enum InterfacePhase
{
	IPHASE_NONE = 0,
	IPHASE_TOP = 1,
	IPHASE_CHECKPW = 2,
	IPHASE_SETPW = 3,
	IPHASE_OPEN = 4,
	IPHASE_CLOSE = 5,
} InterfacePhase_t;

void interface_loop(void);

#endif /* DOOR_INTERFACE_H_ */
