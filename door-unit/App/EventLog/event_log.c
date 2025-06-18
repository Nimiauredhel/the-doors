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

void event_log_append_report_minimal(DoorReport_t report)
{
	event_log_append(PACKET_CAT_REPORT, report, 0, 0);
}

void event_log_append(uint32_t category, uint32_t subcategory, uint16_t data_16, uint32_t data_32)
{
	// TODO: add error code
	if (event_log_length >= EVENT_LOG_CAPACITY) return;

	TAKE_EVENT_LOG_MUTEX;

	RTC_TimeTypeDef now_time = time_get();
	RTC_DateTypeDef now_date = date_get();

	DoorPacket_t *packet_ptr = event_log_buffer + event_log_length;

	packet_ptr->header.category = (DoorPacketCategory_t)category;
	packet_ptr->header.version = 0;
	packet_ptr->header.priority = 0;
	packet_ptr->header.time = packet_encode_time(now_time.Hours, now_time.Minutes, now_time.Seconds);
	packet_ptr->header.date = packet_encode_date(now_date.Year, now_date.Month, now_date.Date);

	switch(packet_ptr->header.category)
	{
	case PACKET_CAT_REPORT:
		packet_ptr->body.Report.source_id = i2c_addr;
		packet_ptr->body.Report.report_id = (DoorReport_t)subcategory;
		packet_ptr->body.Report.report_data_16 = data_16;
		packet_ptr->body.Report.report_data_32 = data_32;
		break;
	case PACKET_CAT_REQUEST:
		packet_ptr->body.Request.request_id = (DoorRequest_t)subcategory;
		packet_ptr->body.Request.source_id = i2c_addr;
		packet_ptr->body.Request.destination_id = data_16;
		packet_ptr->body.Request.request_data_32 = data_32;
		break;
	default:
		break;
	}

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
