#ifndef GUI_ELEMENTS_H
#define GUI_ELEMENTS_H

#include "gui.h"

#define NUM_LAYOUTS (3)

extern InterfaceInputElement_t input_layouts[NUM_LAYOUTS];

void gui_update_door_button(uint8_t index, bool active, char *name);

#endif
