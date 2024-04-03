/*
 * bme280.c
 *
 *  Created on: 18 cze 2023
 *      Author: bluuu
 */

#include "bme280.h"
#include "thp_sensors.h"

uint8_t BME280_check()
{
	uint8_t value;
	TCA9543A_SelectChannel(1);
	osDelay(1);
	HAL_StatusTypeDef status;
	status = HAL_I2C_IsDeviceReady(&hi2c2, BME280_ADDR, 3, 150);
	osDelay(100);
	if (status == HAL_OK) {
		i2c_read8(&hi2c2, BME280_REG_ID, &value, BME280_ADDR);
		TCA9543A_SelectChannel(0);
		if(value == BME280_ID_CHK) {printf("BME280 OK\r\n"); return 1;} else {printf("NOT BME280\r\n"); return 0;}
	} else {printf("BME280 FAILED\r\n"); return 0;}
	return 0;

}


