/*
 * gui.h
 *
 *  Created on: Jun 5, 2025
 *      Author: mickey
 */

#ifndef GUI_H_
#define GUI_H_

#include "common.h"
#include "gfx.h"
#include "audio.h"
#include "utils.h"
#include "client.h"

#include "gui_defs.h"
#include "gui_gfx.h"
#include "gui_touch.h"

int8_t gui_get_touched_button_idx(void);
void gui_task(void *arg);

#endif /* GUI_H_ */
