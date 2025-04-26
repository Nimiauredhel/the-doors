/*
 * hub_comms.c
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#include "hub_comms.h"

#define GENERAL_COMMAND_QUEUE_CAPACITY 24
#define PRIORITY_COMMAND_QUEUE_CAPACITY 8
#define COMMS_DEBUG_LOG_CAPACITY 32

#define TAKE_COMMAND_QUEUE_MUTEX \
	if (is_isr()) xSemaphoreTakeFromISR(command_queues_lock, 0); \
	else xSemaphoreTake(command_queues_lock, portMAX_DELAY)
#define GIVE_COMMAND_QUEUE_MUTEX \
	if (is_isr()) xSemaphoreGiveFromISR(command_queues_lock, 0); \
	else xSemaphoreGive(command_queues_lock)

static bool comms_debug_enabled = false;
static CommsEvent_t comms_debug_log[COMMS_DEBUG_LOG_CAPACITY];
static uint16_t comms_debug_pending_count = 0;

static uint16_t outbox_event_count = 0;

static SemaphoreHandle_t command_queues_lock = NULL;
static StaticSemaphore_t command_queues_lock_buffer;
static uint8_t general_command_queue_head = 0;
static uint8_t priority_command_queue_head = 0;
static uint8_t general_command_queue_len = 0;
static uint8_t priority_command_queue_len = 0;
static DoorPacket_t general_command_queue[GENERAL_COMMAND_QUEUE_CAPACITY] = {0};
static DoorPacket_t priority_command_queue[PRIORITY_COMMAND_QUEUE_CAPACITY] = {0};

static void comms_debug_output(void)
{
	char buff[64] = {0};
	uint8_t count = 0;

	if (comms_debug_enabled)
	{
		uint16_t outbox_current_length = event_log_get_length();

		if (outbox_event_count < outbox_current_length)
		{
			outbox_event_count = outbox_current_length;
			sprintf(buff, "Event Log Outbox has %u events.", outbox_event_count);
			serial_print_line(buff, 0);
		}
		else if (outbox_event_count > 0 && outbox_current_length == 0)
		{
			outbox_event_count = 0;
			serial_print_line("Log has been cleared!", 0);
		}
	}

	if (comms_debug_pending_count > 0)
	{
		count = comms_debug_pending_count;
		comms_debug_pending_count = 0;

		if (comms_debug_enabled)
		{
			for (int i = 0; i < count; i++)
			{
				sprintf(buff, "I2C reports: %s event on %s.",
						comms_debug_log[i].action == COMMS_EVENT_SENT ? "TX" : "RX",
						i2c_register_names[comms_debug_log[i].subject]);
				serial_print_line(buff, 0);
			}
		}
	}
}

static void comms_process_command(DoorPacket_t *cmd_ptr)
{
	switch (cmd_ptr->body.Request.request_id)
	{
	case PACKET_REQUEST_NONE:
		break;
	case PACKET_REQUEST_DOOR_CLOSE:
		door_set_closed(true);
		break;
	case PACKET_REQUEST_DOOR_OPEN:
		door_set_closed(true);
		break;
	case PACKET_REQUEST_BELL:
		serial_print_line("Received bell command, this is unusual.", 0);
		break;
	case PACKET_REQUEST_PHOTO:
		// TODO: implement the photo request
		serial_print_line("Received photo request, yet unimplemented.", 0);
		break;
	case PACKET_REQUEST_SYNC_TIME:
		date_time_set_from_packet(cmd_ptr->header.date, cmd_ptr->header.time);
		break;
	}
}

static void comms_check_command_queue(void)
{
	uint8_t cmd_idx = 0;
	DoorPacket_t *cmd_ptr = NULL;

	TAKE_COMMAND_QUEUE_MUTEX;

	// process entire priority queue before proceeding
	while (priority_command_queue_len > 0)
	{
		cmd_idx = priority_command_queue_head + priority_command_queue_len;
		if (cmd_idx > PRIORITY_COMMAND_QUEUE_CAPACITY) cmd_idx -= PRIORITY_COMMAND_QUEUE_CAPACITY;
		cmd_ptr = priority_command_queue+cmd_idx;
		priority_command_queue_head++;
		if (priority_command_queue_head > PRIORITY_COMMAND_QUEUE_CAPACITY) priority_command_queue_head -= PRIORITY_COMMAND_QUEUE_CAPACITY;
		priority_command_queue_len--;
		comms_process_command(cmd_ptr);
	}

	// process one item at a time from the general queue to avoid blocking
	if (general_command_queue_len > 0)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
		cmd_idx = general_command_queue_head + general_command_queue_len;
		if (cmd_idx > GENERAL_COMMAND_QUEUE_CAPACITY) cmd_idx -= GENERAL_COMMAND_QUEUE_CAPACITY;
		cmd_ptr = general_command_queue+cmd_idx;
		general_command_queue_head++; if (general_command_queue_head > GENERAL_COMMAND_QUEUE_CAPACITY) general_command_queue_head -= GENERAL_COMMAND_QUEUE_CAPACITY;
		general_command_queue_len--;
		comms_process_command(cmd_ptr);
	}

	GIVE_COMMAND_QUEUE_MUTEX;
}

void comms_init(void)
{
	command_queues_lock = xSemaphoreCreateMutexStatic(&command_queues_lock_buffer);
	i2c_io_init();
	serial_print_line("Initialized I2C listening to hub unit.", 0);
}

void comms_loop(void)
{
	vTaskDelay(pdMS_TO_TICKS(50));
	comms_check_command_queue();
	comms_debug_output();
}

void comms_enqueue_command(DoorPacket_t *cmd_ptr)
{
	uint8_t tail_idx;

	TAKE_COMMAND_QUEUE_MUTEX;

	// TODO: this code is begging for a reusable packet queue implementation
	if (cmd_ptr->header.priority > 0)
	{
		if (priority_command_queue_len >= PRIORITY_COMMAND_QUEUE_CAPACITY) return;
		tail_idx = priority_command_queue_head + priority_command_queue_len;
		if (tail_idx >= PRIORITY_COMMAND_QUEUE_CAPACITY) tail_idx -= PRIORITY_COMMAND_QUEUE_CAPACITY;
		memcpy(priority_command_queue+tail_idx, cmd_ptr, sizeof(DoorPacket_t));
		priority_command_queue_len++;
	}
	else
	{
		if (general_command_queue_len >= GENERAL_COMMAND_QUEUE_CAPACITY) return;
		tail_idx = general_command_queue_head + general_command_queue_len;
		if (tail_idx >= GENERAL_COMMAND_QUEUE_CAPACITY) tail_idx -= GENERAL_COMMAND_QUEUE_CAPACITY;
		memcpy(general_command_queue+tail_idx, cmd_ptr, sizeof(DoorPacket_t));
		general_command_queue_len++;
	}

	GIVE_COMMAND_QUEUE_MUTEX;
}

void comms_report_internal(CommsEventType_t action, I2CRegisterDefinition_t subject)
{
	if (!comms_debug_enabled) return;
	// TODO: maybe use a mutex here
	if (comms_debug_pending_count > COMMS_DEBUG_LOG_CAPACITY) return;
	comms_debug_log[comms_debug_pending_count].action = action;
	comms_debug_log[comms_debug_pending_count].subject = subject;
	comms_debug_pending_count++;
}

void comms_toggle_debug(void)
{
	comms_debug_enabled = !comms_debug_enabled;
}
