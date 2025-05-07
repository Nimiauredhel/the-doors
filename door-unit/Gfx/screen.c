/*
 * screen.c
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#include "screen.h"

static uint8_t zerobuff[32] = {255};

uint32_t screen_init(uint32_t orientation)
{
	int32_t ret = 0;
	char buffer[64] = {0};

    serial_print_line("Initializing LCD.\n", 0);

	ret = BSP_LCD_Init(0, orientation);
	sprintf(buffer, "LCD Init result: %ld.\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	uint32_t xs, ys;
	xs = screen_get_x_size();
	ys = screen_get_y_size();
	sprintf(buffer, "Screen XY sizes: %lu, %lu\r\n", xs, ys);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	// clear out the screen
	memset(zerobuff, 255, sizeof(zerobuff));
	screen_fill_rect_loop(zerobuff, sizeof(zerobuff), 0, 0, xs, ys);

	vTaskDelay(pdMS_TO_TICKS(100));

	ret = BSP_LCD_DisplayOff(0);
	sprintf(buffer, "Display Off result: %ld\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	vTaskDelay(pdMS_TO_TICKS(100));

	ret = BSP_LCD_DisplayOn(0);
	sprintf(buffer, "Display On result: %ld\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	return ret;
}

bool screen_fill_rect_loop(uint8_t *data, uint32_t data_length, uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height)
{
	if (width * height < 1) return false;

    uint32_t divisor = (width * height * 2 / data_length);

	int32_t ret = BSP_LCD_SetDisplayWindow(0, x_origin, y_origin, width, height);

	// TODO: figure out enabling DMA
    if(divisor <= 2)
    {
    	ret = BSP_LCD_WriteData(0, data, data_length/2);

    	ret = BSP_LCD_WriteData(0, data+data_length/2, data_length/2);
    }
    else
    {
		for (uint32_t idx = 0; idx < divisor; idx++)
		{
			ret = BSP_LCD_WriteData(0, data, data_length);
		}
    }

    return true;
}

uint32_t screen_get_x_size(void)
{
	uint32_t value;
	BSP_LCD_GetXSize(0, &value);
	return value;
}

uint32_t screen_get_y_size(void)
{
	uint32_t value;
	BSP_LCD_GetYSize(0, &value);
	return value;
}
