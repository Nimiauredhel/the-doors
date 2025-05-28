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
#include "cmsis_os2.h"

#include "main.h"

#include "gfx.h"
#include "audio.h"
#include "uart_io.h"

#include "display.h"
#include "user_auth.h"
#include "door_control.h"
#include "hub_comms.h"

extern SPI_HandleTypeDef hspi5;

typedef enum InterfacePhase
{
	IPHASE_NONE = 0,
	IPHASE_TOP = 1,
	IPHASE_ADMIN = 2,
	IPHASE_CHECKPW_USER = 3,
	IPHASE_CHECKPW_ADMIN = 4,
	IPHASE_SETPW = 5,
	IPHASE_OPEN = 6,
	IPHASE_CLOSE = 7,
	IPHASE_BELL = 8,
	IPHASE_SETTIME = 9,
} InterfacePhase_t;

typedef struct InterfaceTouchState
{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t current_x;
	uint16_t current_y;
} InterfaceTouchState_t;

typedef struct InterfaceButton
{
	uint8_t id;
	uint8_t width;
	uint8_t height;
	uint16_t x;
	uint16_t y;
	char label[8];
} InterfaceButton_t;

extern const InterfaceButton_t keypad_buttons[12];

bool interface_is_initialized(void);
void interface_init(void);
void interface_loop(void);
bool interface_is_msg_dirty(void);
bool interface_is_input_dirty(void);
bool interface_is_keypad_dirty(void);
const char* interface_get_msg(void);
const char* interface_get_input(void);
int8_t interface_get_touched_button_idx(void);

#endif /* USER_INTERFACE_H_ */
