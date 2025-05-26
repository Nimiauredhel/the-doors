/*
 * display.c
 *
 *  Created on: May 7, 2025
 *      Author: mickey
 */

#include "display.h"

static GfxWindow_t *datetime_window = NULL;

GfxWindow_t *msg_window = NULL;
GfxWindow_t *keypad_window = NULL;

bool display_initialized = false;

void display_draw_datetime(void)
{
	static const uint8_t text_scale = 2;
	static const uint8_t font_width = 8*text_scale;
	static const uint8_t font_height = 5*text_scale;

	// initialized at nonsensical value to ensure first draw
	static RTC_TimeTypeDef prev_time = {255, 255, 255};

	char buff[16] = {0};

	if (time_get().Minutes == prev_time.Minutes) return;

	if (gfx_select_window(datetime_window, false))
	{
		prev_time = time_get();
		gfx_fill_screen(color_blue);
		date_time_get_time_str_hhmm(buff);
		gfx_print_string(buff, 2, font_height/2, color_cyan, text_scale);
		date_time_get_date_str(buff);
		gfx_print_string(buff, screen_get_x_size()-2-(strlen(buff) * font_width), font_height/2, color_cyan, text_scale);
		gfx_unselect_window(datetime_window);
	}
}

void display_init(void)
{
	static const uint8_t font_height = 10;

	gfx_init(LCD_ORIENTATION_PORTRAIT_ROT180);

	datetime_window = gfx_create_window(0, 0, screen_get_x_size(), font_height*2, "DateTime");
	msg_window = gfx_create_window(0, font_height*2, screen_get_x_size(), (screen_get_y_size()/2)-(font_height*2), "MsgBox");
	keypad_window = gfx_create_window(0, screen_get_y_size()/2, screen_get_x_size(), screen_get_y_size()/2, "Keypad");
	gfx_show_window(datetime_window);
	gfx_show_window(msg_window);
	gfx_show_window(keypad_window);

	display_initialized = true;

	xpt2046_spi(&hspi5);
	xpt2046_init();
}

void display_loop(void)
{
	display_draw_datetime();
	gfx_refresh();
	vTaskDelay(pdMS_TO_TICKS(16));

	char buff[32] = {0};

	uint16_t x = 0;
	uint16_t y = 0;

	xpt2046_read_position(&x, &y);
	//sprintf(buff, "%03u : %03u : %03u : %03u", ts_CoordinatesRaw.x, ts_CoordinatesRaw.y, ts_CoordinatesRaw.z1, ts_CoordinatesRaw.z2);
	//sprintf(buff, "%03u : %03u : %03u", ts_Coordinates.x, ts_Coordinates.y, ts_Coordinates.z);
	//sprintf(buff, "%03u : %03u", x, y);
	//serial_print_line(buff, 0);
	//event_log_append(PACKET_REPORT_ERROR, x, y);
}
