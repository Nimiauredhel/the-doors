/*
 * door_auth.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "door_auth.h"

// since a password is 4 digits it can fit in a 16-bit int (digit=nibble)
static uint16_t password = 0x0000;
static bool is_auth = false;

static uint16_t str_to_pass(char *rx_msg)
{
	uint16_t idx = 0;
	uint16_t in_pass = 0x00;

	for (idx = 0; idx < 4; idx++)
	{
		in_pass |= (rx_msg[idx] - '0') << idx;
	}

	return in_pass;
}

void auth_reset_auth(void)
{
	is_auth = false;
}

bool auth_is_auth(void)
{
	return is_auth;
}

void auth_check_password(char *rx_msg)
{
	uint16_t in_pass = str_to_pass(rx_msg);
	if (in_pass == password) is_auth = true;
}

void auth_set_password(char *rx_msg)
{
	uint16_t in_pass = str_to_pass(rx_msg);
	password = in_pass;
}
