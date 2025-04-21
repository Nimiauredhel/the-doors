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

#include "i2c_io.h"
#include "uart_io.h"
#include "event_log.h"
#include "packet_defs.h"
#include "i2c_register_defs.h"

void comms_init(void);
void comms_loop(void);

#endif /* HUB_COMMS_H_ */
