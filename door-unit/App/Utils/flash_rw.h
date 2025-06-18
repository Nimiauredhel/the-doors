/*
 * flash_rw.h
 *
 *  Created on: Oct 16, 2024
 *      Author: mickey
 */

#ifndef FLASH_RW_H_
#define FLASH_RW_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f7xx_hal.h"

#define BUFFER_CHAR_LENGTH 32
#define BUFFER_MAX_INDEX 24999
// the above translates to 800kb

void flash_erase_sector(uint32_t sector_to_erase);
void flash_read(uint32_t srcAdr, uint8_t *dstPtr, uint16_t length_bytes);
void flash_write(uint32_t dstAdr, uint8_t *srcPtr, uint16_t length_bytes);

#endif /* FLASH_RW_H_ */
