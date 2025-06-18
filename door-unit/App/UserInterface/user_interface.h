/*
 * door_interface.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_

#include <door_date_time.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "cmsis_os2.h"

#include "main.h"

#include "gfx.h"
#include "audio.h"
#include "uart_io.h"

#include "display.h"
#include "user_auth.h"
#include "door_control.h"
#include "hub_comms.h"

#include "gui_elements.h"

#define INTERFACE_NUM_PHASES (11)
#define INTERFACE_NUM_KEYPADS (2)

typedef enum InterfacePhase
{
	IPHASE_NONE = 0,
	IPHASE_TOP = 1,
	IPHASE_CHECKPW_USER = 2,
	IPHASE_OPEN = 3,
	IPHASE_CLOSE = 4,
	IPHASE_BELL = 5,
	IPHASE_CHECKPW_ADMIN = 6,
	IPHASE_ADMIN_MENU = 7,
	IPHASE_ADMIN_SETPW_USER = 8,
	IPHASE_ADMIN_SETADDR = 9,
	IPHASE_ADMIN_SETNAME = 10,
} InterfacePhase_t;

typedef enum InterfaceKeypadIdx
{
	IKEYPAD_NONE = -1,
	IKEYPAD_DIGITS = 0,
	IKEYPAD_KEYBOARD = 1,
} InterfaceKeypadIdx_t;

typedef struct InterfaceTouchState
{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t current_x;
	uint16_t current_y;
} InterfaceTouchState_t;

extern SPI_HandleTypeDef hspi5;

bool interface_is_initialized(void);
void interface_init(void);
void interface_loop(void);
bool interface_is_msg_dirty(void);
bool interface_is_input_dirty(void);
bool interface_is_keypad_dirty(void);
bool interface_is_keypad_enabled(void);
uint8_t interface_get_input_timer_percent(void);
const char* interface_get_msg(void);
const char* interface_get_input(void);
int8_t interface_get_touched_button_idx(void);
InterfaceKeypadIdx_t interface_get_current_keypad_idx(void);

#endif /* USER_INTERFACE_H_ */
