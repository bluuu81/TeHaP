/*
 * dps368.c
 *
 *  Created on: 20 cze 2023
 *      Author: bluuu
 */

#include "dps368.h"
#include "thp_sensors.h"

uint8_t DPS368_check()
{
	uint8_t value;
	TCA9543A_SelectChannel(2);
	HAL_Delay(1);
	HAL_StatusTypeDef status;
	status = HAL_I2C_IsDeviceReady(&hi2c2, DPS368_ADDR, 3, 150);
	HAL_Delay(100);
	if (status == HAL_OK) {
		i2c_read8(&hi2c2, DPS368_REG_ID, &value, DPS368_ADDR);
		TCA9543A_SelectChannel(0);
		if(value == DPS368_ID_CHK) {printf("DPS368 OK\r\n"); return 1;} else {printf("NOT DPS368\r\n"); return 0;}
	} else {printf("DPS368 FAILED\r\n"); return 0;}
	return 0;
}
