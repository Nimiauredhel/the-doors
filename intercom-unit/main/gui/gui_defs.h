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
    IACTION_RESET         = 1,
    IACTION_BACK          = 2,
    IACTION_OPEN          = 3,
    IACTION_CAMERA        = 4,
    IACTION_SELECT_DOOR_0 = 128,
    IACTION_SELECT_DOOR_1 = 129,
    IACTION_SELECT_DOOR_2 = 130,
    IACTION_SELECT_DOOR_3 = 131,
    IACTION_SELECT_DOOR_4 = 132,
    IACTION_SELECT_DOOR_5 = 133,
    IACTION_SELECT_DOOR_6 = 134,
    IACTION_SELECT_DOOR_7 = 135,
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
	const uint8_t button_count;
	const InterfaceButton_t *buttons;
} InterfaceInputElement_t;

#endif
