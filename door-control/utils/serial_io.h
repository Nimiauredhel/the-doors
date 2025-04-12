/*
 * serial_io.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef SERIAL_IO_H_
#define SERIAL_IO_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os2.h"

#include "main.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern osMutexId_t serial_input_mutexHandle;
extern osMutexId_t serial_output_mutexHandle;

void serial_print(const char *msg, uint16_t len);
void serial_print_line(const char *msg, uint16_t len);
void serial_print_char(const char c);
uint8_t serial_scan(char *buffer, const uint8_t max_len);

#endif /* SERIAL_IO_H_ */
