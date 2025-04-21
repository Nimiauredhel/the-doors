/*
 * i2c_register_defs.h
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#ifndef I2C_REGISTER_DEFS_H_
#define I2C_REGISTER_DEFS_H_

typedef enum I2CRegisterDefinition
{
	I2C_REG_EVENT_COUNT = 0,
	I2C_REG_EVENT_HEAD = 1,
	I2C_REG_QUERY_RESULT = 2,
	I2C_REG_HUB_COMMAND = 3,
	I2C_REG_DATA = 4,
} I2CRegisterDefinition_t;

#endif /* I2C_REGISTER_DEFS_H_ */
