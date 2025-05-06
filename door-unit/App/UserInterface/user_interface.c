/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "user_interface.h"

#define MAX_CHARS_FREE_INPUT 16
#define MAX_CHARS_PASS_INPUT 4
#define PHASE_QUEUE_SIZE 8

static const uint8_t phase_char_limits[6] =
{
		MAX_CHARS_FREE_INPUT, // phase NONE
		MAX_CHARS_FREE_INPUT, // phase TOP
		MAX_CHARS_PASS_INPUT, // phase CHECKPW
		MAX_CHARS_PASS_INPUT, // phase SETPW
		MAX_CHARS_PASS_INPUT, // phase OPEN
		MAX_CHARS_PASS_INPUT, // phase CLOSE
};

static const char *phase_prompts[6] =
{
		"Unknown Phase",
		"1)open\r\r\n2)close\r\r\n3)setpw\r\r\n4)settime\r\r\n5)debug_comms\r\r\n6)debug_sensor",
		"Please Enter Password.",
		"Please Enter NEW Password.",
		"Opening Door!",
		"Closing Door!",
};

static InterfacePhase_t phase_queue[PHASE_QUEUE_SIZE] = {0};

static bool phase_just_reset = false;
static uint8_t phase_queue_index = 0;
static uint8_t phase_queue_tail = 0;
static GfxWindow_t *msg_window = NULL;
static GfxWindow_t *datetime_window = NULL;
static GfxWindow_t *keypad_window = NULL;

static void phase_reset()
{
	if (phase_just_reset) return;

	serial_print_line("Returning to interface top level.", 0);
	bzero(phase_queue, PHASE_QUEUE_SIZE);
	phase_queue[0] = IPHASE_TOP;
	phase_queue_index = 0;
	phase_queue_tail = 0;
	phase_just_reset = true;
}

static void phase_increment()
{
	phase_queue_index++;
	//serial_print_line("Index++.", 0);

	if (phase_queue_index > phase_queue_tail)
	{
		//serial_print_line("Phase queue tail overtaken (this is normal).", 0);
		phase_reset();
	}
	else if (phase_queue_index >= PHASE_QUEUE_SIZE
	 || phase_queue_tail >= PHASE_QUEUE_SIZE)
	{
		serial_print_line("ERROR: Interface max depth exceeded by index.", 0);
		phase_reset();
	}
}

static void phase_push(InterfacePhase_t new_phase)
{
	phase_queue_tail++;
	//serial_print_line("Tail++.", 0);

	if (phase_queue_tail < phase_queue_index)
	{
		serial_print_line("ERROR: tail was incremented, but is lower than index.", 0);
		phase_reset();
	}
	else if (phase_queue_tail >= PHASE_QUEUE_SIZE)
	{
		serial_print_line("ERROR: Interface max depth exceeded by tail.", 0);
		phase_reset();
	}
	else
	{
		phase_queue[phase_queue_tail] = new_phase;
	}
}

static void rx_evaluate(const char *rx_msg)
{
	switch (phase_queue[phase_queue_index])
	{
	case IPHASE_TOP:
		if (strcmp(rx_msg, "open") == 0)
		{
			if (door_is_closed())
			{
				phase_push(IPHASE_CHECKPW);
				phase_push(IPHASE_OPEN);
			}
			else serial_print_line("Command Redundant.", 0);
		}
		else if (strcmp(rx_msg, "close") == 0)
		{
			if (!door_is_closed())
			{
				phase_push(IPHASE_CHECKPW);
				phase_push(IPHASE_CLOSE);
			}
			else serial_print_line("Command Redundant.", 0);
		}
		else if (strcmp(rx_msg, "setpw") == 0)
		{
			phase_push(IPHASE_CHECKPW);
			phase_push(IPHASE_SETPW);
		}
		else if (strcmp(rx_msg, "settime") == 0)
		{
			phase_push(IPHASE_CHECKPW);
			phase_push(IPHASE_SETTIME);
		}
		else if (strcmp(rx_msg, "debug_comms") == 0)
		{
			comms_toggle_debug();
		}
		else if (strcmp(rx_msg, "debug_sensor") == 0)
		{
			door_sensor_toggle_debug();
		}
		else
		{
			serial_print_line("Unknown Command.", 0);
			phase_reset();
		}
		break;
	case IPHASE_CHECKPW:
		auth_check_password(rx_msg);
		break;
	case IPHASE_SETPW:
		auth_set_password(rx_msg);
		auth_reset_auth();
		break;
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
	case IPHASE_NONE:
		phase_reset();
		break;
	case IPHASE_SETTIME:
		// handled externally, shouldn't get here
		break;
	}
}

static void draw_keypad(void)
{
	static const uint8_t x_start = 17;
	static const uint8_t key_width = 68;
	static const uint8_t key_height = 40;
	static const uint8_t digit_scale = 4;
	static const uint8_t digit_width = 8 * digit_scale;
	static const uint8_t digit_height = 5 * digit_scale;

	static const char digits[12][2] =
	{
			"1\0", "2\0", "3\0",
			"4\0", "5\0", "6\0",
			"7\0", "8\0", "9\0",
			"*\0", "0\0", "#\0",
	};
	// 120x120
	while (keypad_window->state != GFXWIN_CLEAN)
	vTaskDelay(pdMS_TO_TICKS(1));
	gfx_select_window(keypad_window);
	keypad_window->state = GFXWIN_WRITING;
	gfx_fill_screen(color_blue);
	for (uint8_t i = 0; i < 12; i++)
	{
		gfx_fill_rect_single_color(
				x_start + (key_width * (i % 3)) + (key_width/8),
				(key_height * (i / 3)) + (key_height/8),
				key_width - (key_width/4),
				key_height - (key_height/4),
				color_white);
		gfx_print_string(digits[i], x_start + (key_width * (i % 3)) + ((key_width - digit_width)/2), (key_height * (i / 3)) + ((key_height - digit_height)/2), color_black, digit_scale);
	}
	keypad_window->state = GFXWIN_DIRTY;
}

static void draw_datetime(void)
{
	static const uint8_t text_scale = 2;
	static const uint8_t font_width = 8*text_scale;
	static const uint8_t font_height = 5*text_scale;

	char buff[64] = {0};

	while (datetime_window->state != GFXWIN_CLEAN)
	vTaskDelay(pdMS_TO_TICKS(1));
	gfx_select_window(datetime_window);
	datetime_window->state = GFXWIN_WRITING;
	gfx_fill_screen(color_blue);
	date_time_get_time_str_hhmm(buff);
	gfx_print_string(buff, 2, font_height/2, color_cyan, text_scale);
	date_time_get_date_str(buff);
	gfx_print_string(buff, screen_get_x_size()-2-(strlen(buff) * font_width), font_height/2, color_cyan, text_scale);
	datetime_window->state = GFXWIN_DIRTY;
}

void interface_init(void)
{
	static const uint8_t font_height = 10;
	while(!door_control_is_init()) vTaskDelay(pdMS_TO_TICKS(1));

	gfx_init(LCD_ORIENTATION_PORTRAIT_ROT180);
	datetime_window = gfx_create_window(0, 0, screen_get_x_size(), font_height*2);
	msg_window = gfx_create_window(0, font_height*2, screen_get_x_size(), (screen_get_y_size()/2)-(font_height*2));
	keypad_window = gfx_create_window(0, screen_get_y_size()/2, screen_get_x_size(), screen_get_y_size()/2);
	gfx_show_window(datetime_window);
	gfx_show_window(msg_window);
	gfx_show_window(keypad_window);
	phase_reset();

	vTaskDelay(1);

	char debug_buff[32];
	sprintf(debug_buff, "Size of header: %d", sizeof(DoorPacketHeader_t));
	serial_print_line(debug_buff, 0);
	sprintf(debug_buff, "Size of body: %d", sizeof(DoorPacketBody_t));
	serial_print_line(debug_buff, 0);
	sprintf(debug_buff, "Size of packet: %d", sizeof(DoorPacket_t));
	serial_print_line(debug_buff, 0);

	vTaskDelay(1);
}

void interface_loop(void)
{
	static const uint8_t text_scale = 2;
	static const uint8_t font_width = 8*text_scale;
	static const uint8_t font_height = 5*text_scale;

	char input[MAX_CHARS_FREE_INPUT] = {0};
	InterfacePhase_t current_phase = phase_queue[phase_queue_index];

	if (phase_just_reset)
	{
		date_time_print();
		draw_datetime();
	}

	gfx_select_window(msg_window);

	vTaskDelay(1);
	phase_just_reset = false;

	switch (current_phase)
	{
	case IPHASE_NONE:
		phase_reset();
		break;
	case IPHASE_TOP:
		while (msg_window->state != GFXWIN_CLEAN)
		vTaskDelay(pdMS_TO_TICKS(1));
		msg_window->state = GFXWIN_WRITING;
		gfx_fill_screen(color_black);
		gfx_print_string(phase_prompts[phase_queue[phase_queue_index]], 0, 2, color_white, text_scale);
		msg_window->state = GFXWIN_DIRTY;
		serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
		draw_keypad();
		serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
		rx_evaluate(input);
		break;
	case IPHASE_CHECKPW:
		if (auth_is_auth())
		{
			while (msg_window->state != GFXWIN_CLEAN)
			vTaskDelay(pdMS_TO_TICKS(1));
			msg_window->state = GFXWIN_WRITING;
			gfx_fill_screen(color_black);
			gfx_print_string("Auth already granted, skipping password check.", 0, 2, color_white, text_scale);
			msg_window->state = GFXWIN_DIRTY;
			serial_print_line("Auth already granted, skipping password check.", 0);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		else
		{
			while (msg_window->state != GFXWIN_CLEAN)
			vTaskDelay(pdMS_TO_TICKS(1));
			msg_window->state = GFXWIN_WRITING;
			gfx_fill_screen(color_black);
			gfx_print_string(phase_prompts[phase_queue[phase_queue_index]], 0, 2, color_white, text_scale);
			msg_window->state = GFXWIN_DIRTY;
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			draw_keypad();
			serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
			rx_evaluate(input);
		}
		break;
	case IPHASE_SETPW:
		if (auth_is_auth())
		{
			while (msg_window->state != GFXWIN_CLEAN)
			vTaskDelay(pdMS_TO_TICKS(1));
			msg_window->state = GFXWIN_WRITING;
			gfx_fill_screen(color_black);
			gfx_print_string(phase_prompts[phase_queue[phase_queue_index]], 0, 2, color_white, text_scale);
			msg_window->state = GFXWIN_DIRTY;
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			draw_keypad();
			serial_scan(input, phase_char_limits[phase_queue[phase_queue_index]]);
			rx_evaluate(input);
		}
		else
		{
			while (msg_window->state != GFXWIN_CLEAN)
			vTaskDelay(pdMS_TO_TICKS(1));
			msg_window->state = GFXWIN_WRITING;
			gfx_fill_screen(color_black);
			gfx_print_string("Cannot change password without authentication.", 0, 2, color_white, text_scale);
			msg_window->state = GFXWIN_DIRTY;
			serial_print_line("Cannot change password without authentication.", 0);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		break;
	case IPHASE_SETTIME:
		if (auth_is_auth())
		{
			date_time_set_interactive();
			auth_reset_auth();
		}
		else
		{
			serial_print_line("Cannot set time/date without authentication.", 0);
		}
		phase_reset();
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
		if (door_is_closed() == (current_phase == IPHASE_CLOSE))
		{
			serial_print_line("Command Redundant.", 0);
		}
		else if (auth_is_auth())
		{
			serial_print_line(phase_prompts[current_phase], 0);
			door_set_closed(current_phase == IPHASE_CLOSE);
		}
		else
		{
			serial_print_line("Cannot proceeed without authentication.", 0);
		}
		auth_reset_auth();
		break;
	}

	if (!phase_just_reset) phase_increment();
}
