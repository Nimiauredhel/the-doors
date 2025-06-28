/*
 * i2c_register_defs.h
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#ifndef I2C_REGISTER_DEFS_H_
#define I2C_REGISTER_DEFS_H_

#define I2C_REGISTER_DEF_COUNT (16)

#define I2C_MIN_ADDRESS (16)
#define I2C_MAX_ADDRESS (116)

typedef enum I2CRegisterDefinition
{
	I2C_REG_EVENT_COUNT = 0,
	I2C_REG_EVENT_HEAD = 1,
	I2C_REG_QUERY_RESULT = 2,
	I2C_REG_3 = 3,
	I2C_REG_HUB_COMMAND = 4,
	I2C_REG_5 = 5,
	I2C_REG_6 = 6,
	I2C_REG_7 = 7,
	I2C_REG_DATA = 8,
	I2C_REG_UNKNOWN = 16,
} I2CRegisterDefinition_t;

extern const char i2c_register_names[I2C_REGISTER_DEF_COUNT][16];

#endif /* I2C_REGISTER_DEFS_H_ */
