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

#define BQ25798_ADDR 0x6B


// Checking
uint8_t BQ25798_check()
{
	HAL_StatusTypeDef status;
	uint8_t res;
	printf("Checking LTC4015 ... ");
	for (int i = 0; i < 10; i++) {
		status = HAL_I2C_IsDeviceReady(&hi2c1, BQ25798_ADDR, 3, 1500);
		HAL_Delay(100);
	    if (status == HAL_OK) {
	    	printf("OK !\r\n");
	    	res = 1;
	        break;
	    } else {
	    	printf("not ready\r\n");
	    	res = 0;
	    	HAL_Delay(500);
	    }
	}
	return res;
}
