/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_interface.h"

static InterfacePhase_t current_phase = IPHASE_TOP;
static InterfacePhase_t next_phase = IPHASE_NONE;

static const uint8_t phase_char_limits[4] =
{
		5, // phase NONE
		5, // phase TOP
		4, // phase CHECKPW
		4, // phase SETPW
};

static void rx_evaluate(const char *rx_msg)
{
	uint16_t idx = 0;
	uint16_t num = 0x00;

	switch (current_phase)
	{
	case IPHASE_TOP:
		if (strcmp(rx_msg, "open") == 0)
		{
			current_phase = IPHASE_CHECKPW;
			next_phase = IPHASE_OPEN;
		}
		else if (strcmp(rx_msg, "close") == 0)
		{
			door_set_state(true);
			auth_reset_auth();
		}
		else if (strcmp(rx_msg, "setpw") == 0)
		{
			current_phase = IPHASE_CHECKPW;
			next_phase = IPHASE_SETPW;
		}
		break;
	case IPHASE_CHECKPW:
		auth_check_password(rx_msg);

		if (auth_is_auth())
		{
			if (next_phase == IPHASE_OPEN)
			{
				door_set_state(false);
				current_phase = IPHASE_TOP;
			}
			else
			{
				current_phase = next_phase;
			}

			next_phase = IPHASE_NONE;
		}
		break;
	case IPHASE_SETPW:
		if (auth_is_auth())
		{
			auth_set_password(rx_msg);
		}

		current_phase = IPHASE_TOP;
		next_phase = IPHASE_NONE;
		break;

	}
}

void interface_loop(void)
{
  uint8_t inchar = ' ';
  uint8_t input_idx = 0;
  uint8_t input[16];

  bool set_pw = false;

  for(;;)
  {
	  if (HAL_OK == HAL_UART_Receive(&huart3, &inchar, 1, 0x00))
	  {
		  if (input_idx >= phase_char_limits[current_phase] || inchar == '\n' || inchar == '\r')
		  {
			  input[input_idx] = '\0';
			  input_idx++;
			  HAL_UART_Transmit(&huart3, (uint8_t *)"\n\r", 2, 0xff);
			  rx_evaluate((char *)input);
			  input_idx = 0;
			  inchar = ' ';
		  }
		  else
		  {
			  HAL_UART_Transmit(&huart3, (uint8_t *)&inchar, 1, 0xff);
			  input[input_idx] = inchar;
			  input_idx++;
		  }
	  }
  }
}
