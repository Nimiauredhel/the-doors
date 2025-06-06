/*
 * utils.h
 *
 *  Created on: Mar 29, 2025
 *      Author: mickey
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

float lerp(float a, float b, float f);
float lerp_simple(float a, float b, float f);
uint16_t map_uint16(uint16_t src_min, uint16_t src_max, uint16_t dst_min, uint16_t dst_max, uint16_t val);

#endif /* UTILS_H_ */
