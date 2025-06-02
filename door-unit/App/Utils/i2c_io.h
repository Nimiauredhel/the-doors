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

#include "event_log.h"
#include "packet_defs.h"
#include "hub_comms.h"
#include "i2c_register_defs.h"

#define I2C_TX_BUFF_SIZE 256
#define I2C_RX_BUFF_SIZE 256
// TODO: decide if a dedicated tx buff is necessary

extern I2C_HandleTypeDef hi2c1;

void i2c_io_init(void);
uint16_t i2c_io_get_device_id(void);
uint32_t i2c_get_addr_hit_counter(void);

#endif /* I2C_IO_H_ */
