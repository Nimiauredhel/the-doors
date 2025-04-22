/*
 * i2c_register_defs.c
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#include "i2c_register_defs.h"

const char i2c_register_names[I2C_REGISTER_DEF_COUNT][16] =
{
		"Event Count",
		"Event Head",
		"Query Result",
		"Hub Command",
		"Data",
		"Unknown",
};
