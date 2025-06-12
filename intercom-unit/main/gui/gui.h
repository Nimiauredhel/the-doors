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

#include "gui_gfx.h"
#include "gui_touch.h"

typedef enum InterfacePhase
{
	IPHASE_NONE = 0,
	IPHASE_TOP = 1,
	IPHASE_ADMIN = 2,
	IPHASE_PAIRING = 3,
	IPHASE_BELL = 4,
	IPHASE_DOOR = 5,
	IPHASE_OPENING = 6,
	IPHASE_CAMERA = 7,
} InterfacePhase_t;

typedef struct InterfaceButton
{
	uint8_t id;
	uint8_t width;
	uint8_t height;
	uint16_t x;
	uint16_t y;
	char label[8];
} InterfaceButton_t;

void gui_task(void *arg);

#endif /* GUI_H_ */
