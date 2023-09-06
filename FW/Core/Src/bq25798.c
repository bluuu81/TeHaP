/*
 * bq25798.c
 *
 *  Created on: 18 maj 2023
 *      Author: bluuu
 */

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bq25798.h"
#include "thp_sensors.h"

#define BQ25798_ADDR 0x6B << 1


// Checking
uint8_t BQ25798_check()
{
	HAL_StatusTypeDef status;
	uint8_t res;
	printf("Checking BQ25798 ... ");
	for (int i = 0; i < 10; i++) {
		status = HAL_I2C_IsDeviceReady(&hi2c1, BQ25798_ADDR, 3, 1500);
		HAL_Delay(100);
	    if (status == HAL_OK) {
	    	printf("OK !\r\n");
	    	BQ25798_set_ADC();
	    	res = OK;
	        break;
	    } else {
	    	res = FAULT;
	    	HAL_Delay(500);
	    }
	}
	if(res == FAULT) printf("not ready\r\n");
	return res;
}

void BQ25798_set_ADC()
{
	uint8_t reg;
	reg = 0b10000000;
	i2c_write8(&hi2c1, REG2E_ADC_Control, reg, BQ25798_ADDR);
	HAL_Delay(1);
}

uint16_t BQ25798_Vbat_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG3B_VBAT_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}

uint16_t BQ25798_Vsys_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG3D_VSYS_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}
