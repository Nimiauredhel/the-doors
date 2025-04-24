/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include <user_interface.h>

#define MAX_CHARS_FREE_INPUT 16
#define MAX_CHARS_PASS_INPUT 4
#define PHASE_QUEUE_SIZE 8

static const uint8_t phase_char_limits[6] =
{
		MAX_CHARS_FREE_INPUT, // phase NONE
		MAX_CHARS_FREE_INPUT, // phase TOP
		MAX_CHARS_PASS_INPUT, // phase CHECKPW
		MAX_CHARS_PASS_INPUT, // phase SETPW
		MAX_CHARS_PASS_INPUT, // phase OPEN
		MAX_CHARS_PASS_INPUT, // phase CLOSE
};

static const char *phase_prompts[6] =
{
		"Unknown Phase",
		"Welcome to DOOR. Valid commands: open, close, setpw, debug_comms, debug_sensor.",
		"Please Enter Password.",
		"Please Enter NEW Password.",
		"Opening Door!",
		"Closing Door!",
};

static InterfacePhase_t phase_queue[PHASE_QUEUE_SIZE] = {0};

static bool phase_just_reset = false;
static uint8_t phase_queue_index = 0;
static uint8_t phase_queue_tail = 0;

static void phase_reset()
{
	if (phase_just_reset) return;

	serial_print_line("Returning to interface top level.", 0);
	bzero(phase_queue, PHASE_QUEUE_SIZE);
	phase_queue[0] = IPHASE_TOP;
	phase_queue_index = 0;
	phase_queue_tail = 0;
	phase_just_reset = true;
}

static void phase_increment()
{
	phase_queue_index++;
	//serial_print_line("Index++.", 0);

	if (phase_queue_index > phase_queue_tail)
	{
		//serial_print_line("Phase queue tail overtaken (this is normal).", 0);
		phase_reset();
	}
	else if (phase_queue_index >= PHASE_QUEUE_SIZE
	 || phase_queue_tail >= PHASE_QUEUE_SIZE)
	{
		serial_print_line("ERROR: Interface max depth exceeded by index.", 0);
		phase_reset();
	}
}

static void phase_push(InterfacePhase_t new_phase)
{
	phase_queue_tail++;
	//serial_print_line("Tail++.", 0);

	if (phase_queue_tail < phase_queue_index)
	{
		serial_print_line("ERROR: tail was incremented, but is lower than index.", 0);
		phase_reset();
	}
	else if (phase_queue_tail >= PHASE_QUEUE_SIZE)
	{
		serial_print_line("ERROR: Interface max depth exceeded by tail.", 0);
		phase_reset();
	}
	else
	{
		phase_queue[phase_queue_tail] = new_phase;
	}
}

static void rx_evaluate(const char *rx_msg)
{
	switch (phase_queue[phase_queue_index])
	{
	case IPHASE_TOP:
		if (strcmp(rx_msg, "open") == 0)
		{
			if (door_is_closed())
			{
				phase_push(IPHASE_CHECKPW);
				phase_push(IPHASE_OPEN);
			}
			else serial_print_line("Command Redundant.", 0);
		}
		else if (strcmp(rx_msg, "close") == 0)
		{
			if (!door_is_closed())
			{
				phase_push(IPHASE_CHECKPW);
				phase_push(IPHASE_CLOSE);
			}
			else serial_print_line("Command Redundant.", 0);
		}
		else if (strcmp(rx_msg, "setpw") == 0)
		{
			phase_push(IPHASE_CHECKPW);
			phase_push(IPHASE_SETPW);
		}
		else if (strcmp(rx_msg, "debug_comms") == 0)
		{
			comms_toggle_debug();
		}
		else if (strcmp(rx_msg, "debug_sensor") == 0)
		{
			door_sensor_toggle_debug();
		}
		else
		{
			serial_print_line("Unknown Command.", 0);
			phase_reset();
		}
		break;
	case IPHASE_CHECKPW:
		auth_check_password(rx_msg);
		break;
	case IPHASE_SETPW:
		auth_set_password(rx_msg);
		auth_reset_auth();
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
	case IPHASE_NONE:
		phase_reset();
		break;
	}
}

void interface_init(void)
{
	serial_uart_initialize();
	while(!door_control_is_init()) vTaskDelay(pdMS_TO_TICKS(1));
	phase_reset();
}

void interface_loop(void)
{
	vTaskDelay(1);

	char input[MAX_CHARS_FREE_INPUT];
	InterfacePhase_t current_phase = phase_queue[phase_queue_index];
	phase_just_reset = false;

	switch (current_phase)
	{
	case IPHASE_NONE:
		phase_reset();
		break;
	case IPHASE_TOP:
		serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
		serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
		rx_evaluate(input);
		break;
	case IPHASE_CHECKPW:
		if (auth_is_auth())
		{
			serial_print_line("Auth already granted, skipping password check.", 0);
		}
		else
		{
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
			rx_evaluate(input);
		}
		break;
	case IPHASE_SETPW:
		if (auth_is_auth())
		{
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
			rx_evaluate(input);
		}
		else
		{
			serial_print_line("Cannot change password without authentication.", 0);
		}
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
		if (door_is_closed() == (current_phase == IPHASE_CLOSE))
		{
			serial_print_line("Command Redundant.", 0);
		}
		else if (auth_is_auth())
		{
			serial_print_line(phase_prompts[current_phase], 0);
			door_set_closed(current_phase == IPHASE_CLOSE);
		}
		else
		{
			serial_print_line("Cannot proceeed without authentication.", 0);
		}
		auth_reset_auth();
		break;
	}

	if (!phase_just_reset) phase_increment();
}
