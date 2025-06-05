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
