/*
 * hub_comms.h
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#ifndef HUB_COMMS_H_
#define HUB_COMMS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "packet_defs.h"
#include "i2c_register_defs.h"
#include "i2c_io.h"
#include "uart_io.h"
#include "sw_random.h"

#include "door_control.h"
#include "user_auth.h"
#include "event_log.h"

typedef enum CommsEventType
{
	COMMS_EVENT_NONE = 0,
	COMMS_EVENT_SENT = 1,
	COMMS_EVENT_RECEIVED = 2,
} CommsEventType_t;

typedef struct CommsEvent
{
	CommsEventType_t action;
	I2CRegisterDefinition_t subject;
} CommsEvent_t;

void comms_init(void);
void comms_loop(void);
void comms_enqueue_command(DoorPacket_t *cmd_ptr);
void comms_report_internal(CommsEventType_t action, I2CRegisterDefinition_t subject);
void comms_toggle_debug(void);

#endif /* HUB_COMMS_H_ */
