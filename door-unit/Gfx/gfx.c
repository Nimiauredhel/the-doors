/*
 * gfx.c
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#include "gfx.h"
#include <stdbool.h>

// RGB565 2 byte format: [ggGbbbbB][rrrrRggg]
const Color565_t color_black = { 0b00000000, 0b00000000 };
const Color565_t color_white = { 0b11111111, 0b11111111 };
const Color565_t color_red = { 0b00000000, 0b11111000 };
const Color565_t color_green = { 0b11100000, 0b00000111 };
const Color565_t color_blue = { 0b00011111, 0b00000000 };
const Color565_t color_cyan = { 0b11111111, 0b00000111 };
const Color565_t color_magenta = { 0b00011111, 0b11111000 };
const Color565_t color_yellow = { 0b11100000, 0b11111111 };

const BinarySpriteSheet_t default_font =
{
		.height_pixels = 5,
		.width_bytes = 1,
		.sprite_count = 128,
		.data = font_8x5,
};

static bool initialized = false;

/**
 * @brief Static gfx.c variable listing currently visible windows. Used by the gfx refresh routine.
 * TODO: add support for:
 * TODO: * configurable draw order
 * TODO: * individual refresh rates
 * TODO: * handling overlaps without resorting to one big buffer,
 * TODO:   or figure out how to effectively use one big buffer
 * TODO:   but selectively push parts of it to the screen.
 */
static GfxWindow_t *window_list = NULL;

/**
 * @brief Static gfx.c variable representing currently selected window for gfx operations.
 */
static GfxWindow_t *selected_window = NULL;

void gfx_refresh(void)
{
	if (!initialized) return;
	GfxWindow_t *current = window_list;

	while (current != NULL)
	{
		gfx_push_to_screen(current);
		current = current->next;
	}
}

void gfx_init(uint32_t orientation)
{
	if (initialized) return;
	screen_init(orientation);
	initialized = true;
}

GfxWindow_t *gfx_create_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	uint32_t buffer_size = width*height*2;
	GfxWindow_t *new_window = malloc(sizeof(GfxWindow_t) + buffer_size);
	explicit_bzero(new_window->buffer, buffer_size);
	new_window->state = GFXWIN_CLEAN;
	new_window->size_bytes = buffer_size;
	new_window->x = x;
	new_window->y = y;
	new_window->width = width;
	new_window->height = height;
	return new_window;
}

void gfx_select_window(GfxWindow_t *window)
{
	/**
	 * TEMPORARY use-lock measure
	 * TODO: replace this with a mutex or more thought-out method
	 */
	while (selected_window != NULL
			&& selected_window->state != GFXWIN_CLEAN)
	{
		vTaskDelay(1);
	}

	selected_window = window;
}

void gfx_show_window(GfxWindow_t *window)
{
	window->next = NULL;

	if (window_list == NULL)
	{
		window_list = window;
	}
	else
	{
		GfxWindow_t *current = window_list;
		while(current->next != NULL)
			current = current->next;
		current->next = window;
	}
}

void gfx_hide_window(GfxWindow_t *window)
{
	if (window_list == NULL)
	{
		return;
	}
	else
	{
		GfxWindow_t *current = window_list;
		GfxWindow_t *prev = NULL;

		do
		{
			if (current == window)
			{
				// if this was the HEAD, make its NEXT the new HEAD
				if (prev == NULL)
				{
					window_list = current->next;
				}
				// if PREV exists, make its NEXT the removed node's NEXT
				else
				{
					prev->next = current->next;
				}

				current->next = NULL;
				break;
			}
			prev = current;
			current = current->next;
		}
		while(current->next != NULL);
	}
}

void gfx_push_to_screen(GfxWindow_t *window)
{
	if (window == NULL)
	{
		// no more default full-screen buffer -
		// if we WANT a full screen buffer, it's easy to define one as a window
		return;
	}
	else if (window->state == GFXWIN_DIRTY)
	{
		window->state = GFXWIN_READING;
		screen_fill_rect_loop(window->buffer, window->size_bytes,
				window->x, window->y, window->width, window->height);
		window->state = GFXWIN_CLEAN;
	}
}

void gfx_rgb_to_565_nonalloc(Color565_t dest, uint8_t red_percent, uint8_t green_percent, uint8_t blue_percent)
{
    // RGB565 2 byte format: [ggGbbbbB][rrrrRggg]
    // start with zeroed struct
	dest[0] = 0x0;
	dest[1] = 0x0;

    // set correctly shifted R segment in the second byte, its final location
    dest[1] = INT_PERCENT(red_percent, R565_MAX) << 3;
    // temporarily set unshifted G segment in first byte
    dest[0] = INT_PERCENT(green_percent, G565_MAX);
    // OR shifted upper half of G into second byte
    dest[1] |= (dest[0] >> 3);
    // shift first byte to only leave lower half of G
    dest[0] <<= 5;
    // finally, OR entire B segment into first byte
    dest[0] |= INT_PERCENT(blue_percent, B565_MAX);

    /*
    char log[64];
    sprintf(log, "R%03u|G%03u|B%03u translated to binary.\r\n", red_percent, green_percent, blue_percent);
    HAL_UART_Transmit(&huart3, (uint8_t *)log, strlen(log), 0xff);
    sprintf(log, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*dest[0]));
    HAL_UART_Transmit(&huart3, (uint8_t *)log, strlen(log), 0xff);
    sprintf(log, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*dest[1]));
    HAL_UART_Transmit(&huart3, (uint8_t *)log, strlen(log), 0xff);
    */
}

void gfx_bytes_to_binary_sprite_nonalloc(BinarySprite_t *sprite, uint16_t height_pixels, uint8_t width_bytes, const uint8_t *data)
{
    uint16_t data_size = height_pixels * width_bytes;
    sprite->width_bytes = width_bytes;
    sprite->height_pixels = height_pixels;

    if (data != NULL)
    {
		memcpy(sprite->pixel_mask, data, data_size);
    }
}

BinarySprite_t* gfx_bytes_to_binary_sprite(uint16_t height_pixels, uint8_t width_bytes, const uint8_t *data)
{
    // start with zeroed struct
    uint16_t data_size = height_pixels * width_bytes;
    BinarySprite_t *sprite = malloc(sizeof(BinarySprite_t) + data_size);
    gfx_bytes_to_binary_sprite_nonalloc(sprite, height_pixels, width_bytes, data);
    return sprite;
}

void gfx_fill_screen(const Color565_t fill_color)
{
	if (selected_window == NULL) return;

	uint32_t idx;

	//memset(selected_window->buffer, *((uint16_t *)fill_color), selected_window->size_bytes);

    for (idx = 0; idx < selected_window->size_bytes; idx+=2)
    {
    	selected_window->buffer[idx] = fill_color[0];
    	selected_window->buffer[idx+1] = fill_color[1];
    }
}

void gfx_fill_rect_loop(const uint8_t *data, uint32_t data_length, uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height)
{
	if (selected_window == NULL) return;
	if (width * height < 1) return;

    uint32_t target_size = width * height * 2;
	uint32_t src_idx = 0;
	uint32_t dest_x = x_origin;
	uint32_t dest_y = y_origin;
	uint32_t dest_idx;

	width += x_origin;
	height += y_origin;

    for (uint32_t idx = 0; idx < target_size; idx+=2)
    {
		dest_idx = 2 * (dest_x + (dest_y * selected_window->width));

        selected_window->buffer[dest_idx] = data[src_idx];
        selected_window->buffer[dest_idx+1] = data[src_idx+1];

		src_idx+=2;
        if (src_idx >= data_length) src_idx = 0;

		dest_x++;

		if (dest_x >= width)
		{
			dest_x = x_origin;
			dest_y++;
		}

		if (dest_y >= height) dest_y = y_origin;
    }
}

void gfx_fill_rect_single_color(uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height, const Color565_t fill_color)
{
	if (selected_window == NULL) return;
	if (width * height < 1) return;

    uint32_t buffer_size = width * height * 2;
	uint32_t buffer_divisor = 1;

	if (buffer_size > selected_window->size_bytes)
	{
		buffer_divisor = (buffer_size / selected_window->size_bytes) + 1;
		buffer_size /= buffer_divisor;
	}

	uint32_t dest_x = x_origin;
	uint32_t dest_y = y_origin;
	uint32_t dest_idx;

	width += x_origin;
	height += y_origin;

    for (uint32_t idx = 0; idx < buffer_size; idx+=2)
    {
		dest_idx = 2 * (dest_x + (dest_y * selected_window->width));

		selected_window->buffer[dest_idx] = fill_color[0];
		selected_window->buffer[dest_idx+1] = fill_color[1];

		dest_x++;

		if (dest_x >= width)
		{
			dest_x = x_origin;
			dest_y++;
		}

		if (dest_y >= height) dest_y = y_origin;
    }
}

static void gfx_draw_binary_byte(uint8_t byte, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale)
{
    int8_t start_bit = -1;
    int8_t end_bit = -1;

    for (uint8_t bit = 0; bit < 8; bit++)
    {
		if (byte & (1 << (7 - bit)))
		{
			if (start_bit == -1)
			{
				start_bit = bit;
			}

			end_bit = bit;
		}
		else
		{
			if (start_bit > -1)
			{
				gfx_fill_rect_single_color(x_origin + (scale * start_bit), y_origin, scale * (1 + (end_bit - start_bit)), scale, color);
				start_bit = -1;
				end_bit = -1;
			}
		}
    }

    if (start_bit > -1)
    {
		gfx_fill_rect_single_color(x_origin + (scale * start_bit), y_origin, scale * (8 - start_bit), scale, color);
    }
}

void gfx_draw_binary_sprite_adhoc(uint16_t height_pixels, uint8_t width_bytes, uint8_t *pixel_mask, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale)
{
    // line by line (possibly naive approach)

    for (uint8_t xb = 0; xb < width_bytes; xb++)
    {
        for (uint16_t y = 0; y < height_pixels; y++)
        {
			gfx_draw_binary_byte(pixel_mask[y + (xb * height_pixels)], x_origin + scale * (xb * 8), y_origin + (y * scale), color, scale);
        }
    }
}

void gfx_draw_binary_sprite(BinarySprite_t *sprite, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale)
{
	gfx_draw_binary_sprite_adhoc(sprite->height_pixels, sprite->width_bytes, sprite->pixel_mask, x_origin, y_origin, color, scale);
}

void gfx_print_string(const char *string, uint16_t x_origin, uint16_t y_origin, const Color565_t color, uint8_t scale)
{
	uint16_t x = x_origin;
	uint16_t y = y_origin;
	uint16_t length = strlen(string);
	uint16_t i = 0;

	for (i = 0; i < length; i++)
	{
		if (string[i] == '\r')
		{
			x = x_origin;
		}
		else if (string[i] == '\n')
		{
			y += default_font.height_pixels * scale * 1.25f;
		}
		else
		{
			if(x+(default_font.width_bytes*8*scale) >= GFX_SCREEN_WIDTH)
			{
				x = x_origin;
				y += default_font.height_pixels * scale * 1.25f;
			}

			if (string[i] != ' ')
			{
				gfx_draw_binary_sprite_adhoc(
				default_font.height_pixels, default_font.width_bytes,
				&default_font.data[((uint8_t)string[i])*default_font.height_pixels*default_font.width_bytes],
				x, y, color, scale);
			}

			x += default_font.width_bytes * 8 * scale;
		}
	}

}

void gfx_draw_rect_sprite_565(RectSprite565_t sprite, uint16_t x_origin, uint16_t y_origin);
void gfx_draw_rect_sprite_565_scaled(RectSprite565_t sprite, uint16_t x_origin, uint16_t y_origin, float scale);
