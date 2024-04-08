/*
 * SIM868.c
 *
 *  Created on: Mar 2, 2024
 *      Author: bluuu
 */
#include "SIM868.h"

void SIM_HW_OFF()
{
	SIM_OFF();
}

void GPS_ON()
{
	///HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

void GPS_OFF()
{
	//HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
}
