/*
 * screen.h
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#ifndef SCREEN_H_
#define SCREEN_H_

#include <stdint.h>

#include "../lcd/lcd.h"

#define SCREEN_MAX_TRANSFER 76800

uint32_t screen_init(LCDOrientation_t orientation, esp_lcd_panel_io_color_trans_done_cb_t tx_done_cb, void *user_ctx);
bool screen_fill_rect_loop(uint8_t *data, uint32_t data_length, uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height);
uint32_t screen_get_x_size(void);
uint32_t screen_get_y_size(void);

#endif /* SCREEN_H_ */
