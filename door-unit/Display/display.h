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
#include "door_date_time.h"

extern GfxWindow_t *msg_window;
extern GfxWindow_t *keypad_window;
extern bool display_initialized;

void display_init(void);
void display_loop(void);
void display_draw_datetime(void);

#endif /* DISPLAY_H_ */
