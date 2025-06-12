#ifndef GUI_TOUCH_H
#define GUI_TOUCH_H

#include "esp_lcd_touch_xpt2046.h"

#include "gui.h"
#include "gui_gfx.h"

#define TOUCH_CS_PIN 21

typedef struct InterfaceTouchState
{
	uint16_t last_x;
	uint16_t last_y;
    uint16_t last_z;
    bool is_touched;
} InterfaceTouchState_t;

void gui_touch_init(void);
void gui_touch_update(void);
bool gui_touch_is_touched(void);
uint16_t gui_touch_get_x(void);
uint16_t gui_touch_get_y(void);
uint16_t gui_touch_get_z(void);

#endif
