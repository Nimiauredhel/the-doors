#ifndef GUI_ELEMENTS_H
#define GUI_ELEMENTS_H

#include "user_interface.h"

typedef struct InterfaceButton
{
	uint8_t id;
	uint8_t label_scale;
	uint8_t width;
	uint8_t height;
	uint16_t x;
	uint16_t y;
	char *label;
} InterfaceButton_t;

typedef struct InterfaceInputElement
{
	const uint8_t button_count;
	const InterfaceButton_t *buttons;
} InterfaceInputElement_t;

extern const InterfaceInputElement_t* keypads[2];

#endif
