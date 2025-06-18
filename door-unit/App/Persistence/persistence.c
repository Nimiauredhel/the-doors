/*
 * persistence.c
 *
 *  Created on: Jun 18, 2025
 *      Author: mickey
 */

#include "persistence.h"

#define FLASH_SECTOR_3_ADDRESS 0x08018000 // decimal: 134,316,032
#define FLASH_SECTOR_4_ADDRESS 0x08020000 // decimal: 134,348,800
#define FLASH_SECTOR_5_ADDRESS 0x08040000 // decimal: 134,479,872
#define FLASH_SECTOR_6_ADDRESS 0x08080000 // decimal: 134,742,016
#define FLASH_SECTOR_7_ADDRESS 0x080C0000 // decimal: 135,004,160

static volatile PersistentData_t inmem_latest_data = {0};

void persistence_init(void)
{
	flash_read(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));

	if (inmem_latest_data.fresh_flag)
	{
		flash_erase_sector(4);
		inmem_latest_data.i2c_addr = 118;
		strncpy("Untitled", inmem_latest_data.name, 32);
		inmem_latest_data.user_pass = 0x0000;
		inmem_latest_data.admin_pass = 0x00000000;
		inmem_latest_data.fresh_flag = 0;
		flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	}
}

void persistence_save_i2c_addr(uint32_t addr)
{
	inmem_latest_data.i2c_addr = addr;
	flash_erase_sector(4);
	flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	vTaskDelay(pdMS_TO_TICKS(500));
	HAL_NVIC_SystemReset();
}

uint32_t persistence_get_i2c_addr(void)
{
	return inmem_latest_data.i2c_addr;
}

void persistence_save_name(const char *name)
{
	strncpy(inmem_latest_data.name, name, 32);
	flash_erase_sector(4);
	flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	vTaskDelay(pdMS_TO_TICKS(500));
}

void persistence_get_name(char *return_buffer)
{
	strncpy(return_buffer, inmem_latest_data.name, 32);
}
