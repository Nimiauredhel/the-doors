/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_interface.h"

static InterfacePhase_t current_phase = IPHASE_TOP;
static InterfacePhase_t next_phase = IPHASE_NONE;

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

static void rx_evaluate(const char *rx_msg)
{
	switch (current_phase)
	{
	case IPHASE_TOP:
		if (strcmp(rx_msg, "open") == 0)
		{
			next_phase = IPHASE_OPEN;
		}
		else if (strcmp(rx_msg, "close") == 0)
		{
			next_phase = IPHASE_CLOSE;
		}
		else if (strcmp(rx_msg, "setpw") == 0)
		{
			next_phase = IPHASE_SETPW;
		}
		break;
	case IPHASE_CHECKPW:
		auth_check_password(rx_msg);

		// if password was rejected, override the previously selected "next phase"
		if (!auth_is_auth())
		{
			next_phase = IPHASE_TOP;
		}
		break;
	case IPHASE_SETPW:
		if (auth_is_auth())
		{
			auth_set_password(rx_msg);
		}
		else
		{
			serial_print_line("Authentication Issue.", 0);
		}

		next_phase = IPHASE_TOP;
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
	case IPHASE_NONE:
		next_phase = IPHASE_TOP;
		break;

	}
}

void interface_loop(void)
{
  char input[16];

  for(;;)
  {
	  switch (current_phase)
	  {
	  case IPHASE_TOP:
	  case IPHASE_CHECKPW:
	  case IPHASE_SETPW:
		  serial_print_line(phase_prompts[current_phase], 0);
		  serial_scan(input, phase_char_limits[current_phase]);
		  rx_evaluate(input);
		  current_phase = next_phase;
		  next_phase = IPHASE_NONE;
		  break;
	  case IPHASE_OPEN:
	  case IPHASE_CLOSE:
		  if (door_get_state() == (current_phase == IPHASE_CLOSE))
		  {
			  serial_print_line("Command Redundant.", 0);
			  next_phase = IPHASE_NONE;
			  current_phase = IPHASE_TOP;
		  }
		  else if (auth_is_auth())
		  {
			  door_set_state(current_phase == IPHASE_CLOSE);
			  serial_print_line(phase_prompts[current_phase], 0);
			  auth_reset_auth();
			  next_phase = IPHASE_NONE;
			  current_phase = IPHASE_TOP;
		  }
		  else
		  {
			  next_phase = current_phase;
			  current_phase = IPHASE_CHECKPW;
		  }
		  break;
	  case IPHASE_NONE:
		  break;
	  }
  }
}
