/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_interface.h"

#define PHASE_QUEUE_SIZE 8

static const uint8_t phase_char_limits[6] =
{
		5, // phase NONE
		5, // phase TOP
		4, // phase CHECKPW
		4, // phase SETPW
		4, // phase OPEN
		4, // phase CLOSE
};

static const char *phase_prompts[6] =
{
		"Unknown Phase",
		"Welcome to DOOR. You may 'open', 'close', or 'setpw'.",
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
	serial_print_line("Index++.", 0);

	if (phase_queue_index > phase_queue_tail)
	{
		serial_print_line("Phase queue tail overtaken (this is normal).", 0);
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
	serial_print_line("Tail++.", 0);

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
			phase_push(IPHASE_CHECKPW);
			phase_push(IPHASE_OPEN);
		}
		else if (strcmp(rx_msg, "close") == 0)
		{
			phase_push(IPHASE_CHECKPW);
			phase_push(IPHASE_CLOSE);
		}
		else if (strcmp(rx_msg, "setpw") == 0)
		{
			phase_push(IPHASE_CHECKPW);
			phase_push(IPHASE_SETPW);
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
		if (auth_is_auth())
		{
			auth_set_password(rx_msg);
			auth_reset_auth();
		}
		else
		{
			serial_print_line("Authentication Issue.", 0);
		}
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
	case IPHASE_NONE:
		phase_reset();
		break;

	}
}

void interface_loop(void)
{
	char input[16];
	InterfacePhase_t current_phase;

	phase_queue_index = 0;
	phase_queue[0] = IPHASE_TOP;

	for(;;)
	{
		current_phase = phase_queue[phase_queue_index];
		phase_just_reset = false;

		switch (current_phase)
		{
		case IPHASE_NONE:
			phase_reset();
			break;
		case IPHASE_CHECKPW:
			if (auth_is_auth())
			{
				serial_print_line("Auth already granted, skipping password check.", 0);
				break;
			}
			// INTENTIONAL FALL-THROUGH when condition is false
		case IPHASE_TOP:
		case IPHASE_SETPW:
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
			rx_evaluate(input);
			break;
		case IPHASE_OPEN:
		case IPHASE_CLOSE:
			if (door_is_closed() == (current_phase == IPHASE_CLOSE))
			{
				serial_print_line("Command Redundant.", 0);
			}
			else if (auth_is_auth())
			{
				door_set_closed(current_phase == IPHASE_CLOSE);
				serial_print_line(phase_prompts[current_phase], 0);
				auth_reset_auth();
			}
			else
			{
				serial_print_line("Authentication Issue.", 0);
			}
			break;
		}

		if (!phase_just_reset) phase_increment();
	}
}
