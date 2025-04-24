/*
 * log.c
 *
 *  Created on: Apr 20, 2025
 *      Author: mickey
 */

#include "event_log.h"

#define TAKE_EVENT_LOG_MUTEX \
	if (is_isr()) xSemaphoreTakeFromISR(event_log_lock, 0); \
	else xSemaphoreTake(event_log_lock, portMAX_DELAY)

#define GIVE_EVENT_LOG_MUTEX \
	if (is_isr()) xSemaphoreGiveFromISR(event_log_lock, 0); \
	else xSemaphoreGive(event_log_lock)

static SemaphoreHandle_t event_log_lock = NULL;
static StaticSemaphore_t event_log_lock_buffer;

static uint8_t event_log_length = 0;
static uint8_t event_log_length_copy = 0;
static DoorPacket_t event_log_buffer[EVENT_LOG_CAPACITY];

void event_log_initialize(void)
{
	event_log_lock = xSemaphoreCreateMutexStatic(&event_log_lock_buffer);
	explicit_bzero(event_log_buffer, sizeof(event_log_buffer));
	event_log_length = 0;
}

void event_log_clear(void)
{
	if (event_log_length == 0) return;
	TAKE_EVENT_LOG_MUTEX;
	event_log_length = 0;
	GIVE_EVENT_LOG_MUTEX;
}

void event_log_append(DoorReport_t report, uint8_t extra_code)
{
	// TODO: add error code
	if (event_log_length >= EVENT_LOG_CAPACITY) return;

	TAKE_EVENT_LOG_MUTEX;

	// TODO: populate new packet with real values
	event_log_buffer[event_log_length].header.category = PACKET_CAT_REPORT;
	event_log_buffer[event_log_length].header.version = 0;
	event_log_buffer[event_log_length].header.timestamp = 0;
	event_log_buffer[event_log_length].body.Report.source_id = 0;
	event_log_buffer[event_log_length].body.Report.report_id = report;
	event_log_buffer[event_log_length].body.Report.report_data_8 = extra_code;

	event_log_length++;
	GIVE_EVENT_LOG_MUTEX;
}

uint16_t event_log_get_length(void)
{
	TAKE_EVENT_LOG_MUTEX;
	uint8_t length = event_log_length;
	GIVE_EVENT_LOG_MUTEX;
	return length;
}

uint8_t* event_log_get_length_ptr(void)
{
	TAKE_EVENT_LOG_MUTEX;
	event_log_length_copy = event_log_length;
	GIVE_EVENT_LOG_MUTEX;
	return &event_log_length_copy;
}

DoorPacket_t* event_log_get_entry(uint16_t index)
{
	TAKE_EVENT_LOG_MUTEX;
	if (index >= EVENT_LOG_CAPACITY) return NULL;
	DoorPacket_t *ptr = &event_log_buffer[index];
	GIVE_EVENT_LOG_MUTEX;
	return ptr;
}
