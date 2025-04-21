/*
 * i2c_io.h
 *
 *  Created on: Apr 21, 2025
 *      Author: mickey
 */

#ifndef I2C_IO_H_
#define I2C_IO_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os2.h"

#include "main.h"

#include "packet_defs.h"
#include "event_log.h"

#define I2C_TX_BUFF_SIZE 256
#define I2C_RX_BUFF_SIZE 256
// TODO: decide if a dedicated tx buff is necessary

typedef enum I2CRegisterDefinition
{
	I2C_REG_EVENT_COUNT = 0,
	I2C_REG_EVENT_HEAD = 1,
	I2C_REG_QUERY_RESULT = 2,
	I2C_REG_HUB_COMMAND = 3,
	I2C_REG_DATA = 4,
} I2CRegisterDefinition_t;

extern I2C_HandleTypeDef hi2c1;

void i2c_io_init(void);

#endif /* I2C_IO_H_ */
