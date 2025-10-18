/*
 * flash_rw.c
 *
 *  Created on: Oct 16, 2024
 *      Author: mickey
 */

#include "flash_rw.h"

void flash_erase_sector(uint32_t sector_to_erase)
{
	FLASH_EraseInitTypeDef flash_erase_struct = {0};
	uint32_t error_status = 0;
	HAL_StatusTypeDef return_error_status;

	while(HAL_OK != HAL_FLASH_Unlock());

	// filling in the flash erase struct
	flash_erase_struct.TypeErase = FLASH_TYPEERASE_SECTORS;
	flash_erase_struct.Sector = sector_to_erase;
	flash_erase_struct.NbSectors = 1;
    flash_erase_struct.VoltageRange = 2;

	return_error_status = HAL_FLASHEx_Erase(&flash_erase_struct, &error_status);
	// TODO: handle/report error

	while(HAL_OK != HAL_FLASH_Lock());
}

void flash_read(uint32_t srcAdr, uint8_t *dstPtr, uint16_t length_bytes)
{
	while(HAL_OK != HAL_FLASH_Unlock());

    uint8_t *srcPtr = (uint8_t *)srcAdr;

    for (uint16_t i = 0; i < length_bytes; i++)
    {
        dstPtr[i] = srcPtr[i];
    }

	while(HAL_OK != HAL_FLASH_Lock());
}

void flash_write(uint32_t dstAdr, uint8_t *srcPtr, uint16_t length_bytes)
{
	while(HAL_OK != HAL_FLASH_Unlock());

    for (uint32_t i = 0; i < length_bytes; i++)
    {
        HAL_FLASH_Program(TYPEPROGRAM_BYTE, dstAdr + i, srcPtr[i]);
    }

	while(HAL_OK != HAL_FLASH_Lock());
}
