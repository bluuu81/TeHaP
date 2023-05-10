/*
 * thp.h
 *
 *  Created on: May 9, 2023
 *      Author: bluuu
 */

#ifndef INC_THP_H_
#define INC_THP_H_

#include "main.h"

extern ADC_HandleTypeDef hadc1;

extern CRC_HandleTypeDef hcrc;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern DMA_HandleTypeDef hdma_i2c2_rx;
extern DMA_HandleTypeDef hdma_i2c2_tx;
extern DMA_HandleTypeDef hdma_i2c3_rx;
extern DMA_HandleTypeDef hdma_i2c3_tx;

extern IWDG_HandleTypeDef hiwdg;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim16;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

void setLed2(uint8_t bri);
void led2Sweep(uint16_t spd, uint16_t cnt, uint16_t wait);
void PWM_Init_Timers();

#endif /* INC_THP_H_ */
