/*
 * display.h
 *
 *  Created on: May 7, 2025
 *      Author: mickey
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>

#include "gfx.h"
#include "user_interface.h"
#include "door_date_time.h"

#include "xpt2046.h"
#include "i2c_io.h"

extern GfxWindow_t *keypad_window;

bool display_is_initialized(void);
void display_init(void);
void display_loop(void);

#endif /* DISPLAY_H_ */
