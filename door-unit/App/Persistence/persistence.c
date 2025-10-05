/*
 * persistence.c
 *
 *  Created on: Jun 18, 2025
 *      Author: mickey
 */

#include "persistence.h"
#include "packet_defs.h"

#define FLASH_SECTOR_3_ADDRESS 0x08018000 // decimal: 134,316,032
#define FLASH_SECTOR_4_ADDRESS 0x08020000 // decimal: 134,348,800
#define FLASH_SECTOR_5_ADDRESS 0x08040000 // decimal: 134,479,872
#define FLASH_SECTOR_6_ADDRESS 0x08080000 // decimal: 134,742,016
#define FLASH_SECTOR_7_ADDRESS 0x080C0000 // decimal: 135,004,160

#define TAKE_PERSISTENCE_MUTEX \
	if (is_isr()) xSemaphoreTakeFromISR(persistence_lock, 0); \
	else xSemaphoreTake(persistence_lock, portMAX_DELAY)
#define GIVE_PERSISTENCE_MUTEX \
	if (is_isr()) xSemaphoreGiveFromISR(persistence_lock, 0); \
	else xSemaphoreGive(persistence_lock)

static SemaphoreHandle_t persistence_lock = NULL;
static StaticSemaphore_t persistence_lock_buffer;

static volatile PersistentData_t inmem_latest_data = {0};

const PersistentData_t* persistence_get_data_ptr(void)
{
	return (const PersistentData_t *)&inmem_latest_data;
}

void persistence_init(void)
{
	flash_read(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));

	if (inmem_latest_data.fresh_flag)
	{
		flash_erase_sector(4);
		inmem_latest_data.i2c_addr = 118;
		strncpy(inmem_latest_data.name, "Untitled", 32);
		inmem_latest_data.user_pass = 0x0000;
		inmem_latest_data.admin_pass = 0x00000000;
		inmem_latest_data.fresh_flag = 0;
		flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	}

	persistence_lock = xSemaphoreCreateMutexStatic(&persistence_lock_buffer);
}

void persistence_save_i2c_addr(uint32_t addr)
{
	TAKE_PERSISTENCE_MUTEX;
	inmem_latest_data.i2c_addr = addr;
	flash_erase_sector(4);
	flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	// in this case we DO NOT give back the persistence mutex - reset imminent
	vTaskDelay(pdMS_TO_TICKS(500));
	HAL_NVIC_SystemReset();
}

uint32_t persistence_get_i2c_addr(void)
{
	TAKE_PERSISTENCE_MUTEX;
	uint32_t ret = inmem_latest_data.i2c_addr;
	GIVE_PERSISTENCE_MUTEX;
	return ret;
}

void persistence_save_user_pass(uint16_t pass)
{
	TAKE_PERSISTENCE_MUTEX;
	inmem_latest_data.user_pass = pass;
	flash_erase_sector(4);
	flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	GIVE_PERSISTENCE_MUTEX;
}

uint16_t persistence_get_user_pass(void)
{
	TAKE_PERSISTENCE_MUTEX;
	uint16_t ret = inmem_latest_data.user_pass;
	GIVE_PERSISTENCE_MUTEX;
	return ret;
}


void persistence_save_admin_pass(uint32_t pass)
{
	TAKE_PERSISTENCE_MUTEX;
	inmem_latest_data.admin_pass = pass;
	flash_erase_sector(4);
	flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	GIVE_PERSISTENCE_MUTEX;
}

uint32_t persistence_get_admin_pass(void)
{
	TAKE_PERSISTENCE_MUTEX;
	uint32_t ret = inmem_latest_data.admin_pass;
	GIVE_PERSISTENCE_MUTEX;
	return ret;
}

void persistence_save_name(const char *name)
{
	TAKE_PERSISTENCE_MUTEX;
	strncpy(inmem_latest_data.name, name, 32);
	flash_erase_sector(4);
	flash_write(FLASH_SECTOR_4_ADDRESS, (uint8_t *)&inmem_latest_data, sizeof(inmem_latest_data));
	vTaskDelay(pdMS_TO_TICKS(500));
	GIVE_PERSISTENCE_MUTEX;
}

void persistence_get_name(char *return_buffer)
{
	TAKE_PERSISTENCE_MUTEX;
	strncpy(return_buffer, inmem_latest_data.name, UNIT_NAME_MAX_LEN);
	GIVE_PERSISTENCE_MUTEX;
}
