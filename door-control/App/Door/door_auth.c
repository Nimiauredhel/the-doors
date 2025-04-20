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

static uint16_t str_to_pass(const char *rx_msg)
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
	serial_print_line("Auth Status Reset.", 0);
	is_auth = false;
}

bool auth_is_auth(void)
{
	return is_auth;
}

void auth_check_password(const char *rx_msg)
{
	uint16_t in_pass = str_to_pass(rx_msg);
	if (in_pass == password) is_auth = true;
	event_log_append(is_auth ? PACKET_REPORT_PASS_CORRECT : PACKET_REPORT_PASS_WRONG);
	serial_print_line(is_auth ? "Password Accepted." : "Password Rejected.", 0);
}

void auth_set_password(const char *rx_msg)
{
	if (auth_is_auth())
	{
		uint16_t in_pass = str_to_pass(rx_msg);
		serial_print_line("Password Changed.", 0);
		password = in_pass;
		event_log_append(PACKET_REPORT_PASS_CHANGED);
	}
	else
	{
		serial_print_line("Cannot change password without authentication.", 0);
	}
}
