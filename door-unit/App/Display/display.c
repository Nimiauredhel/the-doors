/*
 * display.c
 *
 *  Created on: May 7, 2025
 *      Author: mickey
 */

#include "display.h"

static bool display_initialized = false;

GfxWindow_t *keypad_window = NULL;

static GfxWindow_t *datetime_window = NULL;
static GfxWindow_t *msg_window = NULL;
static GfxWindow_t *input_window = NULL;

static void display_draw_datetime(void)
{
	static const uint8_t text_scale = 2;
	static const uint8_t font_width = 8*text_scale;
	static const uint8_t font_height = 5*text_scale;

	static char buff[16] = {0};
	// initialized at nonsensical value to ensure first draw
	static RTC_TimeTypeDef prev_time = {255, 255, 255};

	RTC_TimeTypeDef now_time = time_get();

	if (now_time.Minutes == prev_time.Minutes) return;

	if (gfx_select_window(datetime_window, false))
	{
		prev_time = now_time;
		gfx_fill_screen(color_blue);
		bzero(buff, sizeof(buff));
		date_time_get_time_str_hhmm(buff);
		gfx_print_string(buff, 2, font_height/2, color_cyan, text_scale);
		date_time_get_date_str(buff);
		gfx_print_string(buff, screen_get_x_size()-2-(strlen(buff) * font_width), font_height/2, color_cyan, text_scale);
		gfx_unselect_window(datetime_window);
	}
}

static void display_draw_msg(void)
{
	static const uint8_t text_scale = 2;

	if (interface_is_msg_dirty())
	{
		gfx_select_window(msg_window, true);
		gfx_fill_screen(color_black);
		gfx_print_string(interface_get_msg(), 0, 2, color_white, text_scale);
		gfx_unselect_window(msg_window);
	}
}

static void display_draw_input(void)
{
	static const uint8_t text_scale = 4;

	if (interface_is_input_dirty())
	{
		gfx_select_window(input_window, true);
		gfx_fill_screen(color_white);
		gfx_print_string(interface_get_input(), 0, 2, color_red, text_scale);
		gfx_unselect_window(input_window);
	}
}

static void display_draw_keypad(void)
{
	static const uint8_t digit_scale = 4;
	static const uint8_t digit_width = 8 * digit_scale;
	static const uint8_t digit_height = 5 * digit_scale;

	if (!interface_is_keypad_dirty()) return;

	int8_t touched_button_idx = interface_get_touched_button_idx();

	// 160x240
	gfx_select_window(keypad_window, true);
	gfx_fill_screen(color_black);

	for (int8_t i = 0; i < 12; i++)
	{
		gfx_fill_rect_single_color(
				keypad_buttons[i].x,
				keypad_buttons[i].y,
				keypad_buttons[i].width,
				keypad_buttons[i].height,
				i == touched_button_idx ? color_blue : color_white);
		gfx_print_string(keypad_buttons[i].label,
						 keypad_buttons[i].x + ((keypad_buttons[i].width - digit_width)/2),
						 keypad_buttons[i].y + ((keypad_buttons[i].height - digit_height)/2),
						 i == touched_button_idx ? color_yellow : color_black,
						 digit_scale);
	}

	gfx_unselect_window(keypad_window);
}

bool display_is_initialized(void)
{
	return display_initialized;
}

void display_init(void)
{
	static const uint8_t font_height = 10;

	gfx_init(LCD_ORIENTATION_PORTRAIT);

	datetime_window = gfx_create_window(0, 0, screen_get_x_size(), font_height*2, "DateTime");
	msg_window = gfx_create_window(0, font_height*2, screen_get_x_size(), (screen_get_y_size()/2)-(font_height*2)-(font_height*4), "MsgBox");
	input_window = gfx_create_window(0, (screen_get_y_size()/2)-(font_height*4), screen_get_x_size(), font_height*4, "InputBox");
	keypad_window = gfx_create_window(0, screen_get_y_size()/2, screen_get_x_size(), screen_get_y_size()/2, "Keypad");
	gfx_show_window(datetime_window);
	gfx_show_window(msg_window);
	gfx_show_window(input_window);
	gfx_show_window(keypad_window);

	display_initialized = true;
}

void display_loop(void)
{
	if (!interface_is_initialized()) return;

	display_draw_datetime();
	display_draw_msg();
	display_draw_input();
	display_draw_keypad();
	gfx_refresh();
	vTaskDelay(pdMS_TO_TICKS(16));
}
