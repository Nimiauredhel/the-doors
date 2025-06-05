/*
 * app.h
 *
 *  Created on: Mar 29, 2025
 *      Author: mickey
 */

#ifndef APP_H_
#define APP_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "gfx.h"
#include "audio.h"
#include "utils.h"
#include "pitches.h"

void app_init(void);
void app_loop(void);
void app_clean(void);

#endif /* APP_H_ */
