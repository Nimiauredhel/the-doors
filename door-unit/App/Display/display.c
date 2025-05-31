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

static uint8_t display_draw_datetime(void)
{
	static const uint8_t text_scale = 2;
	static const uint8_t font_width = 8*text_scale;
	static const uint8_t font_height = 5*text_scale;

	static char buff[16] = {0};
	// initialized at nonsensical value to ensure first draw
	static RTC_TimeTypeDef prev_time = {255, 255, 255};

	RTC_TimeTypeDef now_time = time_get();

	if (now_time.Minutes == prev_time.Minutes) return 0;

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
		return 1;
	}

	return 0;
}

static uint8_t display_draw_msg(void)
{
	if (interface_is_msg_dirty())
	{
		gfx_select_window(msg_window, true);

		const char *msg = interface_get_msg();

		if (strlen(msg) > 0)
		{
			gfx_fill_screen(color_black);
			gfx_print_string(interface_get_msg(), 5, 11, color_blue, 2);
			gfx_print_string(interface_get_msg(), 4, 10, color_white, 2);
		}
		else
		{
			gfx_fill_screen(color_red_dark);
			gfx_print_string("DOORS", 8, 33, color_blue, 6);
			gfx_print_string("DOORS", 4, 30, color_white, 6);
			gfx_print_string("DOORS", 7, 32, color_black, 6);
		}

		gfx_unselect_window(msg_window);
		return 1;
	}

	return 0;
}

static uint8_t display_draw_input(void)
{
	if (interface_is_input_dirty())
	{
		const char *input = interface_get_input();
		uint8_t len = strlen(input);

		gfx_select_window(input_window, true);

		if (len == 0)
		{
			gfx_fill_screen(color_black);
		}
		else
		{
			uint8_t digit_scale = len <= 4 ? 5 : 3;
			uint8_t digit_width = (8*digit_scale);
			uint16_t start_x = 120 - ((digit_width/2) * len);
			gfx_fill_screen(color_blue_dark);

			if (len > 8)
			{
				gfx_print_string("ERROR", 40, 6, color_blue, 5);
			}
			else if (len <= 4)
			{
				gfx_print_string(input, start_x+(4-(len/2)), 5, color_yellow, digit_scale);
				gfx_print_string(input, start_x, 6, color_blue, digit_scale);
			}
			else
			{
				gfx_print_string(input, start_x+(4-(len/2)), 11, color_white, digit_scale);
				gfx_print_string(input, start_x, 12, color_cyan, digit_scale);
			}
		}

		gfx_unselect_window(input_window);
		return 1;
	}

	return 0;
}

static uint8_t display_draw_keypad(void)
{
	static const uint8_t digit_scale = 4;
	static const uint8_t digit_width = 8 * digit_scale;
	static const uint8_t digit_height = 5 * digit_scale;

	static bool fresh = true;
	static bool prev_enabled = false;
	static uint8_t prev_timer_percent = 0;
	static int8_t key_last_states[12] = {-1};

	bool enabled;
	int8_t touched_button_idx;
	uint8_t timer_percent;

	enabled = interface_is_keypad_enabled();

	if (enabled == prev_enabled && !interface_is_keypad_dirty()) return 0;

	touched_button_idx = enabled ? interface_get_touched_button_idx() : -1;
	timer_percent = enabled ? interface_get_input_timer_percent() : 0;

	// 160x240
	gfx_select_window(keypad_window, true);

	if (fresh)
	{
		gfx_fill_screen(color_black);
	}

	if (fresh || timer_percent != prev_timer_percent)
	{
		gfx_fill_rect_single_color(20, 0, 200, 2, color_grey_dark);
		gfx_fill_rect_single_color(20, 0, 2 * timer_percent, 2, color_red);
		prev_timer_percent = timer_percent;
	}

	for (int8_t i = 0; i < 12; i++)
	{
		if (!fresh && (enabled == prev_enabled) && key_last_states[i] == (i == touched_button_idx))
			continue;

		key_last_states[i] = (i == touched_button_idx);

		gfx_fill_rect_single_color(
				keypad_buttons[i].x,
				keypad_buttons[i].y,
				keypad_buttons[i].width,
				keypad_buttons[i].height,
				enabled ?
				i == touched_button_idx ? color_blue_dark : color_grey_light
				: color_grey_light);
		gfx_print_string(keypad_buttons[i].label,
						 keypad_buttons[i].x + ((keypad_buttons[i].width - digit_width)/2)
						 +1,
						 keypad_buttons[i].y + ((keypad_buttons[i].height - digit_height)/2)
						 +1,
						 enabled ?
						 i == touched_button_idx ? color_black : color_yellow
						 : color_grey_mid,
						 digit_scale);
		gfx_print_string(keypad_buttons[i].label,
						 keypad_buttons[i].x + ((keypad_buttons[i].width - digit_width)/2)
						 -1,
						 keypad_buttons[i].y + ((keypad_buttons[i].height - digit_height)/2)
						 -1,
						 enabled ?
						 i == touched_button_idx ? color_black : color_yellow
						 : color_grey_mid,
						 digit_scale);
		gfx_print_string(keypad_buttons[i].label,
						 keypad_buttons[i].x + ((keypad_buttons[i].width - digit_width)/2),
						 keypad_buttons[i].y + ((keypad_buttons[i].height - digit_height)/2),
						 enabled ?
						 i == touched_button_idx ? color_yellow : color_blue_dark
						 : color_grey_dark,
						 digit_scale);
	}

	fresh = false;
	prev_enabled = enabled;
	gfx_unselect_window(keypad_window);
	return 1;
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

	uint8_t dirt = 0;

	dirt += display_draw_datetime();
	dirt += display_draw_msg();
	dirt += display_draw_input();
	dirt += display_draw_keypad();

	if (dirt > 0) gfx_refresh();

	vTaskDelay(pdMS_TO_TICKS(32));
}
