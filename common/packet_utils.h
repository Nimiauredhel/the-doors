/*
 * packet_utils.h
 *
 *  Created on: Apr 26, 2025
 *      Author: mickey
 */

#ifndef PACKET_UTILS_H_
#define PACKET_UTILS_H_

#include <stdint.h>
#include "packet_defs.h"

uint16_t packet_encode_date(uint8_t year, uint8_t month, uint8_t day);
uint32_t packet_encode_time(uint8_t hour, uint8_t minutes, uint8_t seconds);

uint8_t packet_decode_year(uint16_t date);
uint8_t packet_decode_month(uint16_t date);
uint8_t packet_decode_day(uint16_t date);

uint8_t packet_decode_hour(uint32_t time);
uint8_t packet_decode_minutes(uint32_t time);
uint8_t packet_decode_seconds(uint32_t time);

#endif /* PACKET_UTILS_H_ */
