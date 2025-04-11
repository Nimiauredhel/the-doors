/*
 * serial_io.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "serial_io.h"

void serial_newline(void)
{
	HAL_UART_Transmit(&huart3, (uint8_t *)"\n\r", 2, 0xff);
}

void serial_print(const char *msg, uint16_t len)
{
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&huart3, (uint8_t *)msg, len, 0xFF);
}

void serial_print_line(const char *msg, uint16_t len)
{
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&huart3, (uint8_t *)msg, len, 0xFF);
	serial_newline();
}

void serial_print_char(const char *c)
{
	HAL_UART_Transmit(&huart3, (uint8_t *)c, 1, 0xFF);
}

uint8_t serial_scan(char *buffer, const uint8_t max_len)
{
	static const char backspace = '\b';
	uint8_t inchar = ' ';
	uint8_t input_idx = 0;

	bzero(buffer, max_len);

	for(;;)
	{
		if (HAL_OK == HAL_UART_Receive(&huart3, &inchar, 1, 0x00))
		{
			if (inchar == backspace)
			{
				if (input_idx > 0)
				{
					buffer[input_idx] = '\0';
					serial_print_char(&backspace);
					input_idx--;
				}
				continue;
			}
			else if (input_idx >= max_len || inchar == '\n' || inchar == '\r')
			{
				buffer[input_idx] = '\0';
				serial_newline();
				return input_idx+1;
			}
			else
			{
				buffer[input_idx] = inchar;
				serial_print_char(buffer + input_idx);
				input_idx++;
			}
		}
	}
}
