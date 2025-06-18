/*
 * persistence.h
 *
 *  Created on: Jun 18, 2025
 *      Author: mickey
 */

#ifndef PERSISTENCE_H_
#define PERSISTENCE_H_

#include <stdlib.h>

#include "main.h"

#include "uart_io.h"
#include "flash_rw.h"

typedef struct PersistentData
{
	uint32_t fresh_flag;
	uint32_t i2c_addr;
	uint32_t admin_pass;
	uint16_t user_pass;
	char name[32];
} PersistentData_t;

void persistence_init(void);

void persistence_save_i2c_addr(uint32_t addr);
uint32_t persistence_get_i2c_addr(void);

void persistence_save_name(const char *name);
void persistence_get_name(char *return_buffer);


#endif /* PERSISTENCE_H_ */
