/**
  ******************************************************************************
  * File Name          : Target/lcd_conf.h
  * Description        : This file provides code for the configuration
  *                      of the LCD instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LCD_CONF_H__
#define __LCD_CONF_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
#include "stm32f7xx_nucleo_bus.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* BUS IO Instance handler */
extern  SPI_HandleTypeDef                   hspi1;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* Number of LCD instances */
#define LCD_INSTANCES_NBR                   1U

/* BUS IO Instance handlers */
#define hLCDSPI                             hspi1

/* BUS IO functions */
#define LCD_SPI_Init                        BSP_SPI1_Init
#define LCD_SPI_DeInit                      BSP_SPI1_DeInit
#define LCD_SPI_Send                        BSP_SPI1_Send
#define LCD_SPI_Recv                        BSP_SPI1_Recv
#define LCD_SPI_SendRecv                    BSP_SPI1_SendRecv
#define LCD_SPI_Send_DMA(td, ln)            BSP_ERROR_FEATURE_NOT_SUPPORTED
#define LCD_SPI_Recv_DMA(rd, ln)            BSP_ERROR_FEATURE_NOT_SUPPORTED
#define LCD_SPI_SendRecv_DMA(td, rd, ln)    BSP_ERROR_FEATURE_NOT_SUPPORTED

/* CS Pin mapping */
#define LCD_CS_GPIO_PORT                    GPIOD
#define LCD_CS_GPIO_PIN                     GPIO_PIN_14

/* DCX Pin mapping */
#define LCD_DCX_GPIO_PORT                   GPIOD
#define LCD_DCX_GPIO_PIN                    GPIO_PIN_15

/* RESET Pin mapping */
#define LCD_RESET_GPIO_PORT                 GPIOF
#define LCD_RESET_GPIO_PIN                  GPIO_PIN_12

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* Chip Reset macro definition */
#define LCD_RST_LOW()                       WRITE_REG(GPIOF->BSRR, (uint32_t)GPIO_PIN_12 << 16)
#define LCD_RST_HIGH()                      WRITE_REG(GPIOF->BSRR, GPIO_PIN_12)

/* Chip Select macro definition */
#define LCD_CS_LOW()                        WRITE_REG(GPIOD->BSRR, (uint32_t)GPIO_PIN_14 << 16)
#define LCD_CS_HIGH()                       WRITE_REG(GPIOD->BSRR, GPIO_PIN_14)

/* Data/Command macro definition */
#define LCD_DC_LOW()                        WRITE_REG(GPIOD->BSRR, GPIO_PIN_15)
#define LCD_DC_HIGH()                       WRITE_REG(GPIOD->BSRR, (uint32_t)GPIO_PIN_15 << 16)

/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif
#endif /* __LCD_CONF_H__ */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
