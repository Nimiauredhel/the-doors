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

#define CURRENT_EVENT_LOG_SLOT event_log_buffer[event_log_length]

static SemaphoreHandle_t event_log_lock = NULL;
static StaticSemaphore_t event_log_lock_buffer;

static volatile uint8_t event_log_length = 0;
static volatile uint8_t event_log_length_copy = 0;
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

void event_log_append_minimal(DoorReport_t report)
{
	event_log_append(report, 0, 0);
}

void event_log_append(DoorReport_t report, uint16_t data_16, uint32_t data_32)
{
	// TODO: add error code
	if (event_log_length >= EVENT_LOG_CAPACITY) return;

	TAKE_EVENT_LOG_MUTEX;

	RTC_TimeTypeDef now_time = time_get();
	RTC_DateTypeDef now_date = date_get();

	// TODO: populate new packet with real values
	CURRENT_EVENT_LOG_SLOT.header.category = PACKET_CAT_REPORT;
	CURRENT_EVENT_LOG_SLOT.header.version = 0;
	CURRENT_EVENT_LOG_SLOT.header.priority = 0;
	CURRENT_EVENT_LOG_SLOT.header.time = packet_encode_time(now_time.Hours, now_time.Minutes, now_time.Seconds);
	CURRENT_EVENT_LOG_SLOT.header.date = packet_encode_date(now_date.Year, now_date.Month, now_date.Date);

	CURRENT_EVENT_LOG_SLOT.body.Report.source_id = 0;
	CURRENT_EVENT_LOG_SLOT.body.Report.report_id = report;
	CURRENT_EVENT_LOG_SLOT.body.Report.report_data_16 = data_16;
	CURRENT_EVENT_LOG_SLOT.body.Report.report_data_32 = data_32;

	event_log_length++;
	GIVE_EVENT_LOG_MUTEX;
}

volatile uint16_t event_log_get_length(void)
{
	TAKE_EVENT_LOG_MUTEX;
	volatile uint8_t length = event_log_length;
	GIVE_EVENT_LOG_MUTEX;
	return length;
}

volatile uint8_t* event_log_get_length_ptr(void)
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
