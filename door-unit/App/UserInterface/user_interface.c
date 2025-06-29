/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "user_interface.h"

#define MAX_CHARS_MENU_INPUT 1
#define MAX_CHARS_ADDR_INPUT 2
#define MAX_CHARS_PASS_INPUT 4
#define MAX_CHARS_ADMIN_PASS_INPUT 8
#define MAX_CHARS_NAME_INPUT 16
#define PHASE_QUEUE_SIZE 8

#define TAKE_GUI_MUTEX \
	if (is_isr()) xSemaphoreTakeFromISR(gui_lock, 0); \
	else xSemaphoreTake(gui_lock, portMAX_DELAY)
#define GIVE_GUI_MUTEX \
	if (is_isr()) xSemaphoreGiveFromISR(gui_lock, 0); \
	else xSemaphoreGive(gui_lock)

static SemaphoreHandle_t gui_lock = NULL;
static StaticSemaphore_t gui_lock_buffer;

static StaticTimer_t input_timer_buffer = {0};
static TimerHandle_t input_timer_handle;

static bool interface_initialized = false;
static uint8_t input_timer_percent = 0;
static InterfaceKeypadIdx_t keypad_idx = -1;

static const uint8_t phase_char_limits[INTERFACE_NUM_PHASES] =
{
		MAX_CHARS_MENU_INPUT, // phase NONE
		MAX_CHARS_MENU_INPUT, // phase TOP
		MAX_CHARS_PASS_INPUT, // phase CHECKPW_USER
		MAX_CHARS_PASS_INPUT, // phase OPEN
		MAX_CHARS_PASS_INPUT, // phase CLOSE
		MAX_CHARS_PASS_INPUT, // phase BELL
		MAX_CHARS_ADMIN_PASS_INPUT, // phase CHECKPW_ADMIN
		MAX_CHARS_MENU_INPUT, // phase ADMIN_MENU
		MAX_CHARS_PASS_INPUT, // phase ADMIN_SETPW_USER
		MAX_CHARS_ADDR_INPUT, // phase ADMIN_SETADDR
		MAX_CHARS_NAME_INPUT, // phase ADMIN_SETNAME
};

static const InterfaceKeypadIdx_t phase_keypad_indices[INTERFACE_NUM_PHASES] =
{
		IKEYPAD_NONE, // phase NONE
		IKEYPAD_DIGITS, // phase TOP
		IKEYPAD_DIGITS, // phase CHECKPW_USER
		IKEYPAD_NONE, // phase OPEN
		IKEYPAD_NONE, // phase CLOSE
		IKEYPAD_DIGITS, // phase BELL
		IKEYPAD_DIGITS, // phase CHECKPW_ADMIN
		IKEYPAD_DIGITS, // phase ADMIN_MENU
		IKEYPAD_DIGITS, // phase ADMIN_SETPW_USER
		IKEYPAD_DIGITS, // phase ADMIN_SETADDR
		IKEYPAD_KEYBOARD, // phase ADMIN_SETNAME
};

static const char *phase_prompts[INTERFACE_NUM_PHASES] =
{
		"Unknown\r\nPhase",
		"",
		"Please\r\nEnter\r\nPassword.",
		"Opening\r\nDoor!",
		"Closing\r\nDoor!",
		"Please Select\r\nClient Number.",
		"Please\r\nEnter\r\nADMIN\r\nPassword.",
		"1)open 2)close\r\n3)set door pw\r\n4)debug comms\r\n5)debug sensor\r\n6)set i2c addr\r\n7)set name",
		"Please\r\nEnter\r\nNEW\r\nPassword.",
		"Please\r\nEnter\r\nI2C\r\nAddress.",
		"Please\r\nEnter\r\nDoor\r\nName.",
};

static const char *imsg_command_redundant = "Command\r\nRedundant.";
static const char *imsg_command_unknown = "Unknown\r\nCommand.";
static const char *imsg_auth_already = "Auth already\r\ngranted,\r\nskipping\r\npassword\r\ncheck.";
static const char *imsg_auth_required = "Cannot\r\nproceeed\r\nwithout\r\nauthentication.";


static bool phase_just_reset = false;
static bool touched = false;
static bool msg_is_dirty = true;
static bool input_is_dirty = true;
static bool keypad_is_enabled = false;
static bool keypad_is_dirty = true;

static uint8_t phase_queue_index = 0;
static uint8_t phase_queue_tail = 0;
static int8_t touched_button_idx = -1;

static InterfaceTouchState_t touch_state = {0};
static InterfacePhase_t phase_queue[PHASE_QUEUE_SIZE] = {0};

static char msg_string[128] = {0};
static char msg_string_safe[128] = {0};
static char input_string[16] = {0};
static char input_string_safe[16] = {0};

static void interface_set_msg(const char *new_msg)
{
	TAKE_GUI_MUTEX;
	snprintf(msg_string, sizeof(msg_string), new_msg);
	msg_is_dirty = true;
	GIVE_GUI_MUTEX;
}

static char touch_interpret()
{
	if (keypad_idx >= 0 && touched && touch_state.current_y >= keypad_window->y)
	{
		uint8_t buttons_count = input_layouts[keypad_idx]->button_count;
        InterfaceButton_t const *buttons = input_layouts[keypad_idx]->buttons;
		uint16_t rel_x = touch_state.current_x - keypad_window->x;
		uint16_t rel_y = touch_state.current_y - keypad_window->y;

		for (int i = 0; i < buttons_count; i++)
		{
			if (rel_x > buttons[i].x
			 && rel_y > buttons[i].y
			 && rel_x < buttons[i].x + buttons[i].width
			 && rel_y < buttons[i].y + buttons[i].height)
			{
				if (touched_button_idx != i)
				{
					TAKE_GUI_MUTEX;
					touched_button_idx = i;
					keypad_is_dirty = true;
					GIVE_GUI_MUTEX;
				}

				return buttons[i].id;
			}
		}
	}

	if (touched_button_idx >= 0)
	{
		TAKE_GUI_MUTEX;
		touched_button_idx = -1;
		keypad_is_dirty = true;
		GIVE_GUI_MUTEX;
	}

	return 0;
}

static void touch_update(void)
{
	bool was_touched = touched;
	xpt2046_read_position(&touch_state.current_x, &touch_state.current_y);
	touched = ts_Coordinates.z > 0 && ts_Coordinates.z < 128
			&& (touch_state.current_x > 0 || touch_state.current_y > 0);

	if (touched)
	{
		if (!was_touched)
		{
			touch_state.start_x = touch_state.current_x;
			touch_state.start_y = touch_state.current_y;
		}
	}
	else if (was_touched)
	{

	}
}

static void touch_scan_timer_callback(TimerHandle_t xTimer)
{
	vTimerSetTimerID(xTimer, (void *)pdTRUE);
}

static void touch_scan_update_timer_percent(void)
{
	TickType_t expiry = xTimerGetExpiryTime(input_timer_handle);
	TickType_t count = xTaskGetTickCount();
	TickType_t elapsed = expiry > count ? expiry - count : count - expiry;
	input_timer_percent = (elapsed * 100) / xTimerGetPeriod(input_timer_handle);
}

static int8_t touch_scan(const uint8_t max_len, uint16_t timeout_ms)
{
#define TEST_INPUT_TIMER \
		if (timeout_ms > 0) \
		{ \
			TAKE_GUI_MUTEX; \
			touch_scan_update_timer_percent(); \
			keypad_is_dirty = true; \
			if (pdTRUE == (BaseType_t)pvTimerGetTimerID(input_timer_handle)) \
			{ \
				bzero(input_string, sizeof(input_string)); \
				input_is_dirty = true; \
				keypad_is_enabled = false; \
				GIVE_GUI_MUTEX; \
				return -1; \
			} \
			GIVE_GUI_MUTEX; \
		} \
		vTaskDelay(pdMS_TO_TICKS(1));

#define RESET_INPUT_TIMER \
		xTimerStop(input_timer_handle, 1); \
		vTimerSetTimerID(input_timer_handle, (void *)pdFALSE); \
		if (timeout_ms > 0) \
		{ \
			xTimerChangePeriod(input_timer_handle, pdMS_TO_TICKS(timeout_ms), 1); \
		}

	static const uint8_t down_threshold = 2;
	static const uint8_t up_threshold = 2;

	uint8_t downchar = 0;
	uint8_t inchar = 0;
	uint8_t upchar = 0;
	uint8_t currchar = 0;
	uint8_t input_idx = 0;

	uint8_t down_counter = 0;
	uint8_t up_counter = 0;

	TAKE_GUI_MUTEX;
	bzero(input_string, sizeof(input_string));
	input_is_dirty = true;
	keypad_is_enabled = true;
	keypad_is_dirty = true;
	GIVE_GUI_MUTEX;

	vTaskDelay(pdMS_TO_TICKS(1));
	RESET_INPUT_TIMER;

	for(;;)
	{
		if (input_idx >= max_len)
		{
			TAKE_GUI_MUTEX;
			keypad_is_enabled = false;
			keypad_is_dirty = true;
			GIVE_GUI_MUTEX;
			return input_idx;
		}

		TEST_INPUT_TIMER;

		downchar = 0;
		inchar = 0;
		upchar = 0;
		currchar = 0;
		up_counter = 0;

		do
		{
			TEST_INPUT_TIMER;

			touch_update();
			downchar = touch_interpret();

			if (inchar != 0 && inchar == downchar)
			{
				down_counter++;
			}
			else
			{
				down_counter = 0;
			}

			inchar = downchar;
		} while (down_counter < down_threshold);

		currchar = inchar;
		audio_sfx_touch_down();
		RESET_INPUT_TIMER;

		do
		{
			if (touched)
			{
				upchar = currchar;
				currchar = touch_interpret();
			}
			else up_counter++;

			touch_update();
		} while (up_counter < up_threshold);

		RESET_INPUT_TIMER;

		if (inchar != upchar)
		{
			audio_sfx_touch_up();
			continue;
		}

		switch (inchar)
		{
		case 0:
			audio_sfx_touch_up();
			break;
		case '\b':
			audio_sfx_cancel();
			vTaskDelay(pdMS_TO_TICKS(10));
			if (input_idx > 0)
			{
				TAKE_GUI_MUTEX;
				input_idx--;
				input_string[input_idx] = '\0';
				input_is_dirty = true;
				GIVE_GUI_MUTEX;
			}
			break;
		case '\n':
			audio_sfx_confirm();
			vTaskDelay(pdMS_TO_TICKS(10));
			TAKE_GUI_MUTEX;
			keypad_is_enabled = false;
			keypad_is_dirty = true;
			GIVE_GUI_MUTEX;
			return input_idx;
		default:
			if (input_idx >= max_len)
			{
				break;
			}

			TAKE_GUI_MUTEX;
			input_string[input_idx] = inchar;
			input_idx++;
			input_is_dirty = true;
			GIVE_GUI_MUTEX;
			audio_sfx_confirm();
			if (input_string[input_idx-1] == '*'
			|| input_string[input_idx-1] == '#')
			{
				vTaskDelay(pdMS_TO_TICKS(10));
				TAKE_GUI_MUTEX;
				keypad_is_enabled = false;
				keypad_is_dirty = true;
				GIVE_GUI_MUTEX;
				return input_idx;
			}
			break;
		}
	}
#undef TEST_INPUT_TIMER
#undef RESET_INPUT_TIMER
}

static void phase_reset()
{
	if (phase_just_reset) return;

	serial_print_line("Returning to interface top level.", 0);
	bzero(phase_queue, PHASE_QUEUE_SIZE);
	phase_queue[0] = IPHASE_TOP;
	phase_queue_index = 0;
	phase_queue_tail = 0;
	phase_just_reset = true;
	vTaskDelay(pdMS_TO_TICKS(100));
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

static void input_evaluate(int8_t input_len)
{
	if (input_len <= 0)
	{
		phase_reset();
		return;
	}

	uint8_t num = atoi(input_string);

	switch (phase_queue[phase_queue_index])
	{
	case IPHASE_TOP:
		switch(input_string[0])
		{
		case '*':
			phase_queue[phase_queue_tail] = IPHASE_BELL;
			break;
		case '#':
			phase_queue[phase_queue_tail] = IPHASE_CHECKPW_ADMIN;
			phase_push(IPHASE_ADMIN_MENU);
			break;
		default:
			phase_reset();
			break;
		}
		break;
	case IPHASE_ADMIN_MENU:

		switch (num)
		{
		case 1:
			if (door_is_closed())
			{
				phase_push(IPHASE_CHECKPW_USER);
				phase_push(IPHASE_OPEN);
			}
			else
			{
				serial_print_line(imsg_command_redundant, 0);
				interface_set_msg(imsg_command_redundant);
				vTaskDelay(pdMS_TO_TICKS(500));
			}
			break;
		case 2:
			if (!door_is_closed())
			{
				phase_push(IPHASE_CHECKPW_USER);
				phase_push(IPHASE_CLOSE);
			}
			else
			{
				serial_print_line(imsg_command_redundant, 0);
				interface_set_msg(imsg_command_redundant);
				vTaskDelay(pdMS_TO_TICKS(500));
			}
			break;
		case 3:
			phase_push(IPHASE_CHECKPW_ADMIN);
			phase_push(IPHASE_ADMIN_SETPW_USER);
			break;
		case 4:
			comms_toggle_debug();
			break;
		case 5:
			door_sensor_toggle_debug();
			break;
		case 6:
			phase_push(IPHASE_ADMIN_SETADDR);
			break;
		case 7:
			phase_push(IPHASE_ADMIN_SETNAME);
			break;
		default:
			serial_print_line(imsg_command_unknown, 0);
			interface_set_msg(imsg_command_unknown);
			vTaskDelay(pdMS_TO_TICKS(500));
			phase_reset();
			break;
		}
		break;
	case IPHASE_CHECKPW_USER:
		switch(input_string[0])
		{
		case '*':
			phase_queue[phase_queue_tail] = IPHASE_BELL;
			break;
		case '#':
			phase_queue[phase_queue_tail] = IPHASE_CHECKPW_ADMIN;
			phase_push(IPHASE_ADMIN_MENU);
			break;
		default:
			auth_check_password(input_string, false);
			break;
		}
		break;
	case IPHASE_CHECKPW_ADMIN:
		auth_check_password(input_string, true);
		break;
	case IPHASE_ADMIN_SETPW_USER:
		auth_set_password(input_string);
		auth_reset_auth();
		break;
	case IPHASE_BELL:
		event_log_append(PACKET_CAT_REQUEST, PACKET_REQUEST_BELL, num, 0);
		break;
	case IPHASE_ADMIN_SETADDR:
		persistence_save_i2c_addr((uint32_t)num << (uint32_t)1);
		auth_reset_auth();
		break;
	case IPHASE_ADMIN_SETNAME:
		persistence_save_name(input_string);
		auth_reset_auth();
		comms_send_info();
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
	case IPHASE_NONE:
		phase_reset();
		break;
	}
}

bool interface_is_initialized(void)
{
	return interface_initialized;
}

void interface_init(void)
{
	gui_lock = xSemaphoreCreateMutexStatic(&gui_lock_buffer);

	xpt2046_spi(&hspi5);
	xpt2046_init();

	input_timer_handle = xTimerCreateStatic("InputTimer", 2000, pdFALSE, (void *)pdFALSE, touch_scan_timer_callback, &input_timer_buffer);

	init_audio();
	interface_initialized = true;

	while(!display_is_initialized() || !door_control_is_init())
		vTaskDelay(pdMS_TO_TICKS(1));

	phase_reset();
	vTaskDelay(pdMS_TO_TICKS(100));
}

void interface_loop(void)
{
	vTaskDelay(pdMS_TO_TICKS(10));

	if (phase_just_reset)
	{
		vTaskDelay(pdMS_TO_TICKS(10));

		if (door_is_closed())
		{
			audio_top_phase_jingle();
		}
		else
		{
			audio_still_open_reminder();
		}
	}

	InterfacePhase_t current_phase = phase_queue[phase_queue_index];
	keypad_idx = phase_keypad_indices[current_phase];
	phase_just_reset = false;

	switch (current_phase)
	{
	case IPHASE_NONE:
		break;
	case IPHASE_TOP:
		auth_reset_auth();

		if (door_is_closed())
		{
			phase_push(IPHASE_CHECKPW_USER);
			phase_push(IPHASE_OPEN);
		}
		else
		{
			interface_set_msg(phase_prompts[phase_queue[phase_queue_index]]);
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			input_evaluate(touch_scan(phase_char_limits[phase_queue[phase_queue_index]], 5000));
		}
		break;
	case IPHASE_ADMIN_MENU:
		if (auth_is_admin_auth())
		{
			interface_set_msg(phase_prompts[phase_queue[phase_queue_index]]);
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			input_evaluate(touch_scan(phase_char_limits[phase_queue[phase_queue_index]], 0));
		}
		else
		{
			interface_set_msg(imsg_auth_required);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		break;
	case IPHASE_CHECKPW_USER:
		if (auth_is_user_auth())
		{
			interface_set_msg(imsg_auth_already);
			serial_print_line(imsg_auth_already, 0);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		else
		{
			interface_set_msg(phase_prompts[phase_queue[phase_queue_index]]);
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			input_evaluate(touch_scan(phase_char_limits[phase_queue[phase_queue_index]], 0));
		}
		break;
	case IPHASE_CHECKPW_ADMIN:
		if (auth_is_admin_auth())
		{
			interface_set_msg(imsg_auth_already);
			serial_print_line(imsg_auth_already, 0);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		else
		{
			interface_set_msg(phase_prompts[phase_queue[phase_queue_index]]);
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			input_evaluate(touch_scan(phase_char_limits[phase_queue[phase_queue_index]], 5000));
		}
		break;
	case IPHASE_BELL:
		interface_set_msg(phase_prompts[phase_queue[phase_queue_index]]);
		serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
		input_evaluate(touch_scan(phase_char_limits[phase_queue[phase_queue_index]], 5000));
		break;
	case IPHASE_ADMIN_SETPW_USER:
	case IPHASE_ADMIN_SETADDR:
	case IPHASE_ADMIN_SETNAME:
		if (auth_is_admin_auth())
		{
			interface_set_msg(phase_prompts[phase_queue[phase_queue_index]]);
			serial_print_line(phase_prompts[phase_queue[phase_queue_index]], 0);
			input_evaluate(touch_scan(phase_char_limits[phase_queue[phase_queue_index]], 5000));
		}
		else
		{
			interface_set_msg(imsg_auth_required);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		break;
	case IPHASE_OPEN:
	case IPHASE_CLOSE:
		if (door_is_closed() == (current_phase == IPHASE_CLOSE))
		{
			serial_print_line(imsg_command_redundant, 0);
			interface_set_msg(imsg_command_redundant);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		else if (auth_is_user_auth())
		{
			serial_print_line(phase_prompts[current_phase], 0);
			interface_set_msg(phase_prompts[current_phase]);
			door_set_closed(current_phase == IPHASE_CLOSE);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		else
		{
			serial_print_line(imsg_auth_required, 0);
			interface_set_msg(imsg_auth_required);
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		auth_reset_auth();
		break;
	}

	if (!phase_just_reset) phase_increment();
}

bool interface_is_msg_dirty(void)
{
	TAKE_GUI_MUTEX;
	bool ret = msg_is_dirty;
	GIVE_GUI_MUTEX;
	return ret;
}

bool interface_is_input_dirty(void)
{
	TAKE_GUI_MUTEX;
	bool ret = input_is_dirty;
	GIVE_GUI_MUTEX;
	return ret;
}

bool interface_is_keypad_dirty(void)
{
	TAKE_GUI_MUTEX;
	bool ret = keypad_is_dirty;
	GIVE_GUI_MUTEX;
	return ret;
}

bool interface_is_keypad_enabled(void)
{
	TAKE_GUI_MUTEX;
	bool ret = keypad_is_enabled;
	GIVE_GUI_MUTEX;
	return ret;
}

uint8_t interface_get_input_timer_percent(void)
{
	// TODO: check that this is fine without a mutex
	return input_timer_percent;
}

const char* interface_get_msg(void)
{
	TAKE_GUI_MUTEX;
	msg_is_dirty = false;
	strncpy(msg_string_safe, msg_string, sizeof(msg_string_safe));
	GIVE_GUI_MUTEX;
	return msg_string_safe;
}

const char* interface_get_input(void)
{
	TAKE_GUI_MUTEX;
	input_is_dirty = false;
	strncpy(input_string_safe, input_string, sizeof(input_string_safe));
	GIVE_GUI_MUTEX;
	return input_string_safe;
}

int8_t interface_get_touched_button_idx(void)
{
	if (!interface_initialized) return -1;

	TAKE_GUI_MUTEX;
	int8_t ret = touched_button_idx;
	keypad_is_dirty = false;
	GIVE_GUI_MUTEX;
	return ret;
}

InterfaceKeypadIdx_t interface_get_current_keypad_idx(void)
{
	return keypad_idx;
}
