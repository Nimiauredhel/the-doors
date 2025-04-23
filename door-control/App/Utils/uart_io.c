/*
 * uart_io.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "uart_io.h"

static void serial_backspace_destructive(uint16_t count)
{
	static const uint8_t* backspace = (uint8_t *)"\b \b";
	static const uint8_t len = 3;

	for (uint16_t idx = 0; idx < count; idx++)
	{
		HAL_UART_Transmit(&huart3, backspace, len, 0xff);
		HAL_UART_Transmit(&huart2, backspace, len, 0xff);
	}
}

static void serial_newline(void)
{
	static const uint8_t* newline = (uint8_t *)"\n\r";
	static const uint8_t len = 2;

	HAL_UART_Transmit(&huart3, newline, len, 0xff);
	HAL_UART_Transmit(&huart2, newline, len, 0xff);
}

void serial_print(const char *msg, uint16_t len)
{
	xSemaphoreTake(serial_output_mutexHandle, portMAX_DELAY);
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&huart3, (uint8_t *)msg, len, 0xFF);
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, len, 0xFF);
	vTaskDelay(pdMS_TO_TICKS(2));
	xSemaphoreGive(serial_output_mutexHandle);
}

void serial_print_line(const char *msg, uint16_t len)
{
	xSemaphoreTake(serial_output_mutexHandle, portMAX_DELAY);
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&huart3, (uint8_t *)msg, len, 0xFF);
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, len, 0xFF);
	serial_newline();
	vTaskDelay(pdMS_TO_TICKS(2));
	xSemaphoreGive(serial_output_mutexHandle);
}

void serial_print_char(const char c)
{
	xSemaphoreTake(serial_output_mutexHandle, 0xff);
	HAL_UART_Transmit(&huart3, (uint8_t *)&c, 1, 0xFF);
	HAL_UART_Transmit(&huart2, (uint8_t *)&c, 1, 0xFF);
	vTaskDelay(pdMS_TO_TICKS(2));
	xSemaphoreGive(serial_output_mutexHandle);
}

uint8_t serial_scan(char *buffer, const uint8_t max_len)
{
	uint8_t inchar = ' ';
	uint8_t input_idx = 0;

	bzero(buffer, max_len);

	for(;;)
	{
		vTaskDelay(pdMS_TO_TICKS(1));

		if (HAL_OK == HAL_UART_Receive(&huart3, &inchar, 1, 0x0)
			|| HAL_OK == HAL_UART_Receive(&huart2, &inchar, 1, 0x0))
		{
			switch (inchar)
			{
			case '\b':
				if (input_idx > 0)
				{
					buffer[input_idx] = '\0';
					xSemaphoreTake(serial_output_mutexHandle, portMAX_DELAY);
					serial_backspace_destructive(1);
					xSemaphoreGive(serial_output_mutexHandle);
					input_idx--;
				}
				continue;
			case '\n':
			case '\r':
				buffer[input_idx] = '\0';
				xSemaphoreTake(serial_output_mutexHandle, portMAX_DELAY);
				serial_newline();
				xSemaphoreGive(serial_output_mutexHandle);
				return input_idx+1;
			default:
				if (input_idx >= max_len)
				{
					continue;
				}
				buffer[input_idx] = inchar;
				serial_print_char(inchar);
				input_idx++;
			}
		}
	}
}
