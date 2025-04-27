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

#include "door_date_time.h"
#include "isr_utils.h"
#include "packet_defs.h"
#include "packet_utils.h"

#define EVENT_LOG_CAPACITY 127

void event_log_initialize(void);
void event_log_clear(void);
void event_log_append_minimal(DoorReport_t report);
void event_log_append(DoorReport_t report, uint16_t data_16, uint32_t data_32);
volatile uint16_t event_log_get_length(void);
volatile uint8_t* event_log_get_length_ptr(void);
DoorPacket_t* event_log_get_entry(uint16_t index);

#endif /* EVENT_LOG_H_ */
