/*
 * utils.h
 *
 *  Created on: Mar 29, 2025
 *      Author: mickey
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

float lerp(float a, float b, float f);
float lerp_simple(float a, float b, float f);
uint16_t map_uint16(uint16_t src_min, uint16_t src_max, uint16_t dst_min, uint16_t dst_max, uint16_t val);
void set_datetime(uint16_t hour, uint16_t min, uint16_t sec, uint16_t day, uint16_t month, uint16_t year);
struct tm get_datetime(void);

#endif /* UTILS_H_ */
