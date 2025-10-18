/*
 * door_auth.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "user_auth.h"

// since a door pass is 4 digits it can fit in a 16-bit int (digit=nibble)
static uint16_t user_password = 0x0000;
// the admin pass is 8 digits but follows the same method
static uint32_t admin_password = 0x00000000;

static bool is_user_auth = false;
static bool is_admin_auth = false;

static uint32_t str_to_pass(const char *input_str)
{
	char debug_buff[32] = {0};
	uint8_t idx = 0;
	uint8_t len = strlen(input_str);
	uint32_t in_pass = 0x00000000;

	for (idx = 0; idx < len; idx++)
	{
		uint32_t input = (input_str[idx] - '0') << (idx * 4);
		in_pass |= input;
		snprintf(debug_buff, sizeof(debug_buff), "%lu ", input);
		serial_print(debug_buff, 0);
	}

	serial_print_line(NULL, 0);

	return in_pass;
}

void auth_reset_auth(void)
{
	serial_print_line("Auth Status Reset.", 0);
	is_user_auth = false;
	is_admin_auth = false;
}

bool auth_is_user_auth(void)
{
	return is_user_auth;
}

bool auth_is_admin_auth(void)
{
	return is_admin_auth;
}

void auth_check_password(const char *input_str, bool admin)
{
	uint32_t in_pass = str_to_pass(input_str);

	if ((admin && in_pass == admin_password)
		|| (!admin && in_pass == user_password))
	{
		if (admin)
		{
			is_admin_auth = true;
		}
		else
		{
			is_user_auth = true;
		}

		event_log_append(PACKET_CAT_REPORT, PACKET_REPORT_PASS_CORRECT, in_pass, 0);
		serial_print_line("Password Accepted.", 0);
		audio_success_jingle();
	}
	else
	{
		event_log_append(PACKET_CAT_REPORT, PACKET_REPORT_PASS_WRONG, in_pass, 0);
		serial_print_line("Password Rejected.", 0);
		audio_failure_jingle();
	}

}

void auth_set_password(const char *rx_msg)
{
	if (auth_is_user_auth())
	{
		uint16_t in_pass = str_to_pass(rx_msg);
		serial_print_line("Password Changed.", 0);
		user_password = in_pass;
		event_log_append(PACKET_CAT_REPORT, PACKET_REPORT_PASS_CHANGED, user_password, admin_password);
	}
	else
	{
		serial_print_line("Cannot change password without authentication.", 0);
	}
}

void auth_set_user_pass_internal(uint16_t new_user_pass)
{
	user_password = new_user_pass;
}

void auth_set_admin_pass_internal(uint32_t new_admin_pass)
{
	admin_password = new_admin_pass;
}
