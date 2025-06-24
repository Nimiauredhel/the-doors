#ifndef GUI_DEFS_H
#define GUI_DEFS_H

#include "common.h"
#include "gfx.h"

typedef enum InterfacePhase
{
	IPHASE_NONE = 0,
	IPHASE_TOP = 1,
	IPHASE_ADMIN = 2,
	IPHASE_PAIRING = 3,
	IPHASE_BELL = 4,
	IPHASE_DOOR = 5,
	IPHASE_OPENING = 6,
	IPHASE_CAMERA = 7,
} InterfacePhase_t;

typedef enum InterfaceLayout
{
    ILAYOUT_NONE = -1,
    ILAYOUT_KEYBOARD = 0,
    ILAYOUT_DOOR_LIST = 1,
    ILAYOUT_DOOR_ACTIONS = 2,
} InterfaceLayout_t;

typedef enum InterfaceActions
{
    IACTION_NONE          = 0,
    IACTION_SELECT_DOOR_1 = 1,
    IACTION_SELECT_DOOR_2 = 2,
    IACTION_SELECT_DOOR_3 = 3,
    IACTION_SELECT_DOOR_4 = 4,
    IACTION_SELECT_DOOR_5 = 5,
    IACTION_SELECT_DOOR_6 = 6,
    IACTION_SELECT_DOOR_7 = 7,
    IACTION_SELECT_DOOR_8 = 8,
    IACTION_RESET         = 27,
    IACTION_BACK          = 28,
    IACTION_OPEN          = 29,
    IACTION_CAMERA        = 30,
} InterfaceAction_t;

typedef struct InterfaceButton
{
	uint8_t id;
    uint8_t label_scale;
	uint8_t width;
	uint8_t height;
	uint16_t x;
	uint16_t y;
	char label[8];
    const BinarySprite_t *icon;
} InterfaceButton_t;

typedef struct InterfaceInputElement
{
    const InterfaceLayout_t id;
	const uint8_t button_count;
	const InterfaceButton_t *buttons;
} InterfaceInputElement_t;

#endif
