/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "String.h"
#include "thp.h"
#include "thp_sensors.h"
#include "bq25798.h"
#include "bmp280.h"
#include "dps368.h"
#include "adc.h"
#include "cli.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define But_ONOFF_Pin GPIO_PIN_13
#define But_ONOFF_GPIO_Port GPIOC
#define Tamp_Pin GPIO_PIN_0
#define Tamp_GPIO_Port GPIOA
#define SIM_UART_TX_Pin GPIO_PIN_2
#define SIM_UART_TX_GPIO_Port GPIOA
#define SIM_UART_RX_Pin GPIO_PIN_3
#define SIM_UART_RX_GPIO_Port GPIOA
#define SIM_UART_RI_Pin GPIO_PIN_4
#define SIM_UART_RI_GPIO_Port GPIOA
#define SIM_UART_DTR_Pin GPIO_PIN_5
#define SIM_UART_DTR_GPIO_Port GPIOA
#define SIM_GPS_Pin GPIO_PIN_6
#define SIM_GPS_GPIO_Port GPIOA
#define SIM_PWR_Pin GPIO_PIN_0
#define SIM_PWR_GPIO_Port GPIOB
#define SIM_WDT_Pin GPIO_PIN_1
#define SIM_WDT_GPIO_Port GPIOB
#define TP7_Pin GPIO_PIN_2
#define TP7_GPIO_Port GPIOB
#define TP8_Pin GPIO_PIN_10
#define TP8_GPIO_Port GPIOB
#define BQ_INT_Pin GPIO_PIN_12
#define BQ_INT_GPIO_Port GPIOB
#define RST3_Pin GPIO_PIN_15
#define RST3_GPIO_Port GPIOB
#define RST2_Pin GPIO_PIN_8
#define RST2_GPIO_Port GPIOA
#define BQ_QON_Pin GPIO_PIN_11
#define BQ_QON_GPIO_Port GPIOA
#define BQ_CE_Pin GPIO_PIN_12
#define BQ_CE_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_5
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_8
#define LED2_GPIO_Port GPIOB
#define Main_SW_Pin GPIO_PIN_9
#define Main_SW_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define LED1_ON()		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED1_OFF()		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED1_TOGGLE()	HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin)

#define LED2_ON()		setLed2(31)
#define LED2_OFF()		setLed2(0)
#define LED2_TOGGLE()	{if(TIM16->CCR1 & 0xFFFF) setLed2(0); else setLed2(31);}

#define POWER_ON()		HAL_GPIO_WritePin(Main_SW_GPIO_Port, Main_SW_Pin, GPIO_PIN_SET)
#define POWER_OFF()		HAL_GPIO_WritePin(Main_SW_GPIO_Port, Main_SW_Pin, GPIO_PIN_RESET)

#define Power_SW_READ()	HAL_GPIO_ReadPin(But_ONOFF_GPIO_Port, But_ONOFF_Pin)
#define Tamp_READ()		HAL_GPIO_ReadPin(Tamp_GPIO_Port, Tamp_Pin)

#define BQ_INT_READ()	HAL_GPIO_ReadPin(BQ_INT_GPIO_Port, BQ_INT_Pin)
#define SIM_WDT_READ()	HAL_GPIO_ReadPin(SIM_WDT_GPIO_Port, SIM_WDT_Pin)

#define GPS_ON()		HAL_GPIO_WritePin(SIM_GPS_GPIO_Port, SIM_GPS_Pin, GPIO_PIN_SET)
#define GPS_OFF()		HAL_GPIO_WritePin(SIM_GPS_GPIO_Port, SIM_GPS_Pin, GPIO_PIN_RESET)

#define I2C2TCA_RST()	HAL_GPIO_WritePin(RST2_GPIO_Port, RST2_Pin, GPIO_PIN_RESET)
#define I2C2TCA_NRST()	HAL_GPIO_WritePin(RST2_GPIO_Port, RST2_Pin, GPIO_PIN_SET)
#define I2C3TCA_RST()	HAL_GPIO_WritePin(RST3_GPIO_Port, RST3_Pin, GPIO_PIN_RESET)
#define I2C3TCA_NRST()	HAL_GPIO_WritePin(RST3_GPIO_Port, RST3_Pin, GPIO_PIN_SET)

#define WDR() if(hiwdg.Instance == IWDG) {HAL_IWDG_Refresh(&hiwdg);}


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
