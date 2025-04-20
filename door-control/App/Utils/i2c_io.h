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

extern I2C_HandleTypeDef hi2c1;

#endif /* I2C_IO_H_ */
