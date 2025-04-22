/*
 * hub_comms.c
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#include "hub_comms.h"

#define COMMS_DEBUG_LOG_CAPACITY 32

static bool comms_debug_enabled = false;
static CommsEvent_t comms_debug_log[COMMS_DEBUG_LOG_CAPACITY];
static uint16_t comms_debug_pending_count = 0;

static uint16_t outbox_event_count = 0;

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

void comms_init(void)
{
	i2c_io_init();
	vTaskDelay(pdMS_TO_TICKS(10));
	serial_print_line("I2C listening initialized.", 0);
}

void comms_loop(void)
{
	vTaskDelay(pdMS_TO_TICKS(100));
	comms_debug_output();
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
