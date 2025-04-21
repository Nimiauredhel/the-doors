/*
 * hub_comms.c
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#include "hub_comms.h"

uint16_t log_state = 0;

void comms_init(void)
{
	i2c_io_init();
	while (serial_input_mutexHandle == NULL) vTaskDelay(pdMS_TO_TICKS(1));
}

void comms_loop(void)
{
	char buff[64] = {0};

	vTaskDelay(pdMS_TO_TICKS(100));

	uint16_t log_current_length = event_log_get_length();

	if (log_state < log_current_length)
	{
		log_state = log_current_length;
		sprintf(buff, "Log has %u events.", log_state);
		serial_print_line(buff, 0);
	}
	else if (log_state > 0 && log_current_length == 0)
	{
		log_state = 0;
		serial_print_line("Log has been cleared!", 0);
	}
}
