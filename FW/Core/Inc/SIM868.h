/*
 * SIM868.h
 *
 *  Created on: Mar 2, 2024
 *      Author: bluuu
 */

#include "main.h"

#ifndef INC_SIM868_H_
#define INC_SIM868_H_

#define SIM_ON()		HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_SET)
#define SIM_OFF()		HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_RESET)

#define GPS_ON()		HAL_GPIO_WritePin(SIM_GPS_GPIO_Port, SIM_GPS_Pin, GPIO_PIN_RESET)
#define GPS_OFF()		HAL_GPIO_WritePin(SIM_GPS_GPIO_Port, SIM_GPS_Pin, GPIO_PIN_SET)

#define SIM_WDT_READ()	HAL_GPIO_ReadPin(SIM_WDT_GPIO_Port, SIM_WDT_Pin)


void SIM_HW_OFF();

#endif /* INC_SIM868_H_ */
