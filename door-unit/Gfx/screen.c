/*
 * screen.c
 *
 *  Created on: Mar 23, 2025
 *      Author: mickey
 */

#include "screen.h"

extern SPI_HandleTypeDef hspi3;

static uint8_t zerobuff[320] = {0};

uint32_t screen_init(uint32_t orientation)
{
	int32_t ret;
	char buffer[64];

	ret = BSP_LCD_Init(0, orientation);
	sprintf(buffer, "LCD Init result: %ld.\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	ret = BSP_LCD_SetOrientation(0, LCD_ORIENTATION_LANDSCAPE);
	sprintf(buffer, "Set Orientation result: %ld\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	uint32_t xs, ys;
	xs = screen_get_x_size();
	ys = screen_get_y_size();
	sprintf(buffer, "Screen XY sizes: %lu, %lu\r\n", xs, ys);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	// clear out the screen
	memset(zerobuff, 250, sizeof(zerobuff));
	screen_fill_rect_loop(zerobuff, sizeof(zerobuff), 0, 0, xs, ys);

	ret = BSP_LCD_DisplayOff(0);
	sprintf(buffer, "Display Off result: %ld\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	ret = BSP_LCD_DisplayOn(0);
	sprintf(buffer, "Display On result: %ld\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);

	return ret;
}

void screen_fill_rect_loop(uint8_t *data, uint32_t data_length, uint16_t x_origin, uint16_t y_origin, uint16_t width, uint16_t height)
{
	//char buffer[64] = {0};

	if (width * height < 1) return;

    uint32_t divisor = (width * height * 2 / data_length);

	int32_t ret = BSP_LCD_SetDisplayWindow(0, x_origin, y_origin, width, height);
	/*sprintf(buffer, "Set Window result: %ld\r\n", ret);
	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);*/

    if(divisor <= 2)
    {
    	ret = BSP_LCD_WriteData(0, data, data_length/2);
		/*sprintf(buffer, "Write Data (1/2) result: %ld\r\n", ret);
		HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);*/

    	ret = BSP_LCD_WriteData(0, data+data_length/2, data_length/2);
		/*sprintf(buffer, "Write Data (2/2) result: %ld\r\n", ret);
		HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);*/
    }
    else
    {
		for (uint32_t idx = 0; idx < divisor; idx++)
		{
			// TODO: figure out enabling DMA
			ret = BSP_LCD_WriteData(0, data, data_length);
			/*sprintf(buffer, "Write Data (%lu/%lu) result: %ld\r\n", idx, divisor-1, ret);
			HAL_UART_Transmit(&huart3, (uint8_t *)buffer, strlen(buffer)+1, 0xff);*/
		}
    }
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

void screen_await_transfer(void)
{
	BSP_LCD_WaitForTransferToBeDone(0);
}

// TODO: figure out how to use the tearing callback later
/*void BSP_LCD_SignalTearingEffectEvent(uint32_t Instance, uint8_t state, uint16_t Line)
{
  if(Instance == 0)
  {
    if(state)
    {
      if(Line == 0)
      {
      }
    }
    else
    {
    }
  }
}*/
