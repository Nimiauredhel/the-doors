/*
 * log.c
 *
 *  Created on: Apr 20, 2025
 *      Author: mickey
 */

#include "event_log.h"

static SemaphoreHandle_t event_log_lock = NULL;
static StaticSemaphore_t event_log_lock_buffer;

static uint16_t event_log_length = 0;
static DoorPacket_t event_log_buffer[EVENT_LOG_CAPACITY];

void event_log_initialize(void)
{
	event_log_lock = xSemaphoreCreateMutexStatic(&event_log_lock_buffer);
	explicit_bzero(event_log_buffer, sizeof(event_log_buffer));
	event_log_length = 0;
}

void event_log_append(DoorReport_t report)
{
	// TODO: add error code
	if (event_log_length >= EVENT_LOG_CAPACITY) return;
	xSemaphoreTake(event_log_lock, 0xffff);

	// TODO: populate new packet with real values
	event_log_buffer[event_log_length].header.category = PACKET_CAT_REPORT;
	event_log_buffer[event_log_length].header.version = 0;
	event_log_buffer[event_log_length].header.timestamp = 0;
	event_log_buffer[event_log_length].body.Report.source_id = 0;
	event_log_buffer[event_log_length].body.Report.report_id = report;

	event_log_length++;
	xSemaphoreGive(event_log_lock);
}

uint16_t event_log_get_length(void)
{
	xSemaphoreTake(event_log_lock, 0xffff);
	uint16_t length = event_log_length;
	xSemaphoreGive(event_log_lock);
	return length;
}

DoorPacket_t* event_log_get_entry(uint16_t index)
{
	xSemaphoreTake(event_log_lock, 0xffff);
	if (index >= EVENT_LOG_CAPACITY) return NULL;
	DoorPacket_t *ptr = &event_log_buffer[index];
	xSemaphoreGive(event_log_lock);
	return ptr;
}
