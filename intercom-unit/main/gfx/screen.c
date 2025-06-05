/*
 * screen.c
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#include "screen.h"
#include "esp_lcd_ili9341.h"

static esp_lcd_panel_handle_t panel_handle;

uint32_t screen_init(LCDOrientation_t orientation, esp_lcd_panel_io_color_trans_done_cb_t tx_done_cb, void *user_ctx)
{
	int32_t ret = 0;
	char buffer[64];

    printf("Gfx initializing the screen!\n");

    lcd_initialize_bus();
    lcd_initialize_interface(tx_done_cb, user_ctx);
    panel_handle = lcd_initialize_screen(orientation);

    /*static uint16_t color_buf[240*320] = {0};

    for (int i = 0; i < 76800; i++)
    {
        uint16_t x = i % 320;
        uint16_t y = i / 320;
        memset(color_buf, (uint16_t)(UINT16_MAX * ((float)i/76800.0f)), sizeof(color_buf));
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, x, y, x+1, y+1, color_buf));
    }*/

	// clear out the screen
	explicit_bzero(buffer, 64);
	screen_fill_rect_loop((uint8_t*)buffer, 64, 0, 0, screen_get_x_size(), screen_get_y_size());

    return ret;
}

bool screen_fill_rect_loop(uint8_t *data, uint32_t data_length, uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height)
{
	if (width * height < 1) return false;

    esp_lcd_panel_draw_bitmap(panel_handle, x_origin, y_origin, x_origin+width, y_origin+height, data);
    return true;
}

uint32_t screen_get_x_size(void)
{
	return lcd_get_width();
}

uint32_t screen_get_y_size(void)
{
	return lcd_get_height();
}
