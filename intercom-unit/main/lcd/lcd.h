#ifndef LCD_H
#define LCD_H

#include <string.h>
#include "driver/spi_master.h"
#include "esp_check.h"

#include "esp_lcd_panel_io.h"       // Header file dependency

#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"

typedef enum LCDOrientation
{
    LCD_PORTRAIT = 0,
    LCD_PORTRAIT_FLIP = 1,
    LCD_LANDSCAPE = 2,
    LCD_LANDSCAPE_FLIP = 3
} LCDOrientation_t;

void lcd_initialize_bus(void);
void lcd_initialize_interface(esp_lcd_panel_io_color_trans_done_cb_t tx_done_cb, void *user_ctx);
esp_lcd_panel_handle_t lcd_initialize_screen(LCDOrientation_t orientation);
void lcd_deinitialize_screen(void);
uint16_t lcd_get_width(void);
uint16_t lcd_get_height(void);

#endif
