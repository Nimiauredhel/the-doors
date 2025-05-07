/*
 * gfx.c
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#include "gfx.h"

//#define GFX_PRINT_DEBUG

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
/**
 * @brief Static gfx.c variable representing next window to be transferred to the screen.
 */
static GfxWindow_t *sent_window = NULL;

static StaticSemaphore_t transfer_sem_buffer;
static SemaphoreHandle_t transfer_sem_handle;

bool gfx_refresh(void)
{
	if (!initialized) return false;
	GfxWindow_t *current = window_list;

	while (current != NULL)
	{
        xSemaphoreTake(current->sem_handle, portMAX_DELAY);
		gfx_push_to_screen(current);
		xSemaphoreGive(current->sem_handle);

        current = current->next;
	}

	return true;
}

void gfx_init(uint32_t orientation)
{
	if (initialized) return;

#ifdef GFX_PRINT_DEBUG
    serial_print_line("Initializing graphics library.\n", 0);
#endif
    transfer_sem_handle = xSemaphoreCreateBinaryStatic(&transfer_sem_buffer);
	screen_init(orientation);
    xSemaphoreGive(transfer_sem_handle);
	initialized = true;
}

GfxWindow_t *gfx_create_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *name)
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

    strncpy(new_window->name, name == NULL ? "Untitled Window" : name, 16);

    new_window->sem_handle = xSemaphoreCreateBinaryStatic(&new_window->sem_buff);
    xSemaphoreGive(new_window->sem_handle);

#ifdef GFX_PRINT_DEBUG
    serial_print("Created new window: ", 0);
    serial_print(new_window->name, 0);
    serial_print_line(".", 1);
#endif

	return new_window;
}

void gfx_dispose_window(GfxWindow_t *window)
{
    if (selected_window == window) gfx_unselect_window(window);

    while (sent_window == window) vTaskDelay(pdMS_TO_TICKS(2));

#ifdef GFX_PRINT_DEBUG
    serial_print("Disposing of window: ", 0);
    serial_print(window->name, 0);
    serial_print_line(".", 1);
#endif
    if (!xSemaphoreTake(window->sem_handle, pdMS_TO_TICKS(1000) == pdPASS))
    {
		serial_print("Failed to safely acquire window: ", 0);
		serial_print(window->name, 0);
		serial_print_line(". Disposing anyway (risky!)", 0);
    }
    vSemaphoreDelete(window->sem_handle);
    free(window);
}

bool gfx_select_window(GfxWindow_t *window, bool blocking)
{
    if (blocking)
    {
        while (sent_window == window) vTaskDelay(pdMS_TO_TICKS(2));
        while (xSemaphoreTake(window->sem_handle, 0) != pdPASS) vTaskDelay(pdMS_TO_TICKS(2));
    }
    else
    {
        if (sent_window == window) return false;
        if (xSemaphoreTake(window->sem_handle, 0) != pdPASS) return false;
    }

#ifdef GFX_PRINT_DEBUG
		serial_print("Selected window: ", 0);
		serial_print(window->name, 0);
		serial_print_line(".", 1);
#endif
    selected_window = window;
    selected_window->state = GFXWIN_WRITING;
    xSemaphoreGive(window->sem_handle);

    return true;
}

void gfx_unselect_window(GfxWindow_t *window)
{
    if (xSemaphoreTake(window->sem_handle, 0))
    {
#ifdef GFX_PRINT_DEBUG
		serial_print("Unselected window: ", 0);
		serial_print(window->name, 0);
		serial_print_line(".", 1);
#endif
        selected_window->state = GFXWIN_DIRTY;
        selected_window = NULL;
        xSemaphoreGive(window->sem_handle);
    }
}

void gfx_show_window(GfxWindow_t *window)
{
#ifdef GFX_PRINT_DEBUG
		serial_print("Showing window: ", 0);
		serial_print(window->name, 0);
		serial_print_line(".", 1);
#endif
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
#ifdef GFX_PRINT_DEBUG
		serial_print("Hiding window: ", 0);
		serial_print(window->name, 0);
		serial_print_line(".", 1);
#endif

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

bool gfx_push_to_screen(GfxWindow_t *window)
{
	if (window == NULL)
	{
		// no more default full-screen buffer -
		// if we WANT a full screen buffer, it's easy to define one as a window
		return false;
	}
	else if (window->state == GFXWIN_DIRTY)
	{
        if (selected_window != window && sent_window == NULL)
        {
            xSemaphoreTake(transfer_sem_handle, portMAX_DELAY);
            bool transfer = false;
            window->state = GFXWIN_READING;

            transfer = screen_fill_rect_loop(window->buffer, window->size_bytes,
                    window->x, window->y, window->width, window->height);
            sent_window = window;
            window->state = GFXWIN_CLEAN;

			sent_window = NULL;
            xSemaphoreGive(transfer_sem_handle);

            return transfer;
        }
	}

    return false;
}

void gfx_rgb_to_565_nonalloc(Color565_t dest, uint8_t red_percent, uint8_t green_percent, uint8_t blue_percent)
{
    // RGB565 2 byte format: [RrrrrGgg][gggBbbbb]
    // start with zeroed struct
	dest[1] = 0x00;
	dest[0] = 0x00;

    // set correctly shifted R segment in the second byte, its final location
    dest[0] = INT_PERCENT(red_percent, R565_MAX) << 3;
    // temporarily set unshifted G segment in first byte
    dest[1] = INT_PERCENT(green_percent, G565_MAX);
    // OR shifted upper half of G into second byte
    dest[0] |= (dest[1] >> 3);
    // shift first byte to only leave lower half of G
    dest[1] <<= 5;
    // finally, OR entire B segment into first byte
    dest[1] |= INT_PERCENT(blue_percent, B565_MAX);
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
