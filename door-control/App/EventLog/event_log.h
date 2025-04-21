/*
 * log.h
 *
 *  Created on: Apr 20, 2025
 *      Author: mickey
 */

#ifndef EVENT_LOG_H_
#define EVENT_LOG_H_

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "packet_defs.h"

#define EVENT_LOG_CAPACITY 4096

void event_log_initialize(void);
void event_log_clear(void);
void event_log_append(DoorReport_t report, uint8_t extra_code);
uint16_t event_log_get_length(void);
uint16_t* event_log_get_length_ptr(void);
DoorPacket_t* event_log_get_entry(uint16_t index);

#endif /* EVENT_LOG_H_ */
