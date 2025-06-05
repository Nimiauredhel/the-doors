/*
 * gfx.h
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#ifndef GFX_H_
#define GFX_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "screen.h"
#include "fonts.h"

#include "driver/gptimer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define GFX_SCREEN_WIDTH 320
#define GFX_SCREEN_HEIGHT 240
#define GFX_SCREEN_SIZE_BYTES (GFX_SCREEN_WIDTH * GFX_SCREEN_HEIGHT * 2)

#define R565_MAX 31
#define G565_MAX 63
#define B565_MAX 31

#define INT_PERCENT(percent, max) ((max * percent) / 100)

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\r\n"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

#define SHORT_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n"
#define SHORT_TO_BINARY(byte)  \
  ((byte) & 0x8000 ? '1' : '0'), \
  ((byte) & 0x4000 ? '1' : '0'), \
  ((byte) & 0x2000 ? '1' : '0'), \
  ((byte) & 0x1000 ? '1' : '0'), \
  ((byte) & 0x0800 ? '1' : '0'), \
  ((byte) & 0x0400 ? '1' : '0'), \
  ((byte) & 0x0200 ? '1' : '0'), \
  ((byte) & 0x0100 ? '1' : '0'), \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

// TODO: figure out if this is better or worse than using 'uint8_t *' or 'uint16_t'
typedef uint8_t Color565_t[2];

typedef enum GfxWindowState
{
	GFXWIN_CLEAN = 0,
	GFXWIN_WRITING = 1,
	GFXWIN_DIRTY = 2,
	GFXWIN_READING = 3,
} GfxWindowState_t;

/**
 * @typedef GfxWindow_t
 * @brief GfxWindow_t represents a target region on the screen and an associated pixel buffer.
 * The field widths are restricted to an absurd theoretical max resolution of 4095*4095.
 * The benefits/drawbacks of this are still to be determined.
 * Currently also holds a pointer to another GfxWindow to facilitate a linked list.
 * TODO: Consider extracting the pointer to a separate Node struct or using array indices.
 * TODO: figure out if restricting the field widths has either beneficial or detrimental effects.
 */
typedef struct GfxWindow
{
    StaticSemaphore_t sem_buff;
    SemaphoreHandle_t sem_handle;
	GfxWindowState_t state : 4;
	uint16_t x : 12;
	uint16_t y : 12;
	uint16_t width : 12;
	uint16_t height : 12;
	uint32_t size_bytes : 24;
	uint32_t id;
	struct GfxWindow *next;
    char name[16];
	uint8_t buffer[];
} GfxWindow_t;

/**
 * @typedef BinarySprite_t
 * @brief BinarySprite_t uses individual bits to minimally represent a two-dimensional shape.
 * The specifics of color, position, scale etc. can be decided later by the application.
 */
typedef struct BinarySprite
{
	uint16_t height_pixels;
	uint8_t width_bytes;
	uint8_t pixel_mask[];
} BinarySprite_t;

typedef struct BinarySpriteSheet
{
	uint16_t height_pixels;
	uint8_t width_bytes;
	uint8_t sprite_count;
	uint8_t *data;
} BinarySpriteSheet_t;

// TODO: make a version of this that allows for negative space, like the binary sprite,
// TODO: without making every single cell bigger (like a bool would).
// TODO: a possible solution is variable length enconding for streaks of color/nothing
// TODO: VLE format idea: [VLE byte][color byte 0][color byte 1]
// TODO: VLE byte contents: (1->128) = color repeat length, (129->255) = blank length, (0) = terminator
// TODO: allow up to 127 compressed repeat color and/or repeat blank pixels, and length is encoded by terminator
// This struct represents a width*height rectangle of RGB565 pixels (NO blanks, NO transparency)
typedef struct RectSprite565
{
	uint16_t height_pixels;
	uint16_t width_pixels;
	Color565_t pixel_colors[];
} RectSprite565_t;

extern bool gfx_dirty;

extern const Color565_t color_black;
extern const Color565_t color_white;
extern const Color565_t color_red;
extern const Color565_t color_green;
extern const Color565_t color_blue;
extern const Color565_t color_cyan;
extern const Color565_t color_magenta;
extern const Color565_t color_yellow;

extern const BinarySpriteSheet_t default_font;

void gfx_init(LCDOrientation_t orientation);
uint8_t gfx_get_transfer_count(void);
GfxWindow_t *gfx_create_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *name);
void gfx_dispose_window(GfxWindow_t *window);
bool gfx_select_window(GfxWindow_t *window, bool blocking);
void gfx_unselect_window(GfxWindow_t *window);
void gfx_show_window(GfxWindow_t *window);
void gfx_hide_window(GfxWindow_t *window);
bool gfx_push_to_screen(GfxWindow_t *window);

/**
 * @brief takes in 0-100 percentages of RGB values and maps them to an approximate 565 representation
 * @param destination
 * @param red_percent
 * @param green_percent
 * @param blue_percent
 */
void gfx_rgb_to_565_nonalloc(Color565_t destination, uint8_t red_percent, uint8_t green_percent, uint8_t blue_percent);

void gfx_bytes_to_binary_sprite_nonalloc(BinarySprite_t *sprite, uint16_t height_pixels, uint8_t width_bytes, const uint8_t *data);
BinarySprite_t* gfx_bytes_to_binary_sprite(uint16_t height, uint8_t width, const uint8_t *data);

void gfx_fill_screen(const Color565_t fill_color);
void gfx_fill_rect_loop(const uint8_t *data, uint32_t data_length, uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height);
void gfx_fill_rect_single_color(uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height, const Color565_t fill_color);

void gfx_draw_binary_sprite(BinarySprite_t *sprite, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale);
void gfx_draw_binary_sprite_adhoc(uint16_t height_pixels, uint8_t width_bytes, uint8_t *pixel_mask, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale);

void gfx_print_string(char *string, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale);

void gfx_draw_rect_sprite_565(RectSprite565_t sprite, uint16_t x_origin, uint16_t y_origin);
void gfx_draw_rect_sprite_565_scaled(RectSprite565_t sprite, uint16_t x_origin, uint16_t y_origin, float scale);

#endif /* GFX_H_ */
