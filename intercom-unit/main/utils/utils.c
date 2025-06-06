/*
 * utils.c
 *
 *  Created on: Mar 29, 2025
 *      Author: mickey
 */

#include "utils.h"

float lerp(float a, float b, float f)
{
    return a * (1.0 - f) + (b * f);
}

float lerp_simple(float a, float b, float f)
{
    return a + f * (b - a);
}

uint16_t map_uint16(uint16_t src_min, uint16_t src_max, uint16_t dst_min, uint16_t dst_max, uint16_t val)
{
    uint16_t src_len = src_max - src_min;
    uint16_t dst_len = dst_max - dst_min;
    float slope = (float)dst_len / (float)src_len;
    return dst_min + slope * (val - src_min);
    //return (dst_min + ((dst_max - dst_min) / (src_max - src_min)) * (val - src_min));
    //return dst_min + ((dst_max - dst_min) * (val - src_min)) / (src_max - src_min);
}
