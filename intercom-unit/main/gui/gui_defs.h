#ifndef GUI_DEFS_H
#define GUI_DEFS_H

#include "common.h"

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

typedef struct InterfaceButton
{
	uint8_t id;
    uint8_t label_scale;
	uint8_t width;
	uint8_t height;
	uint16_t x;
	uint16_t y;
	char label[8];
} InterfaceButton_t;


#endif
