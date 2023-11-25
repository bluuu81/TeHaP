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

uint16_t BQ25798_Vbus_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG35_VBUS_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}

uint16_t BQ25798_Vac1_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG37_VAC1_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}

uint16_t BQ25798_Vac2_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG39_VAC2_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}

uint16_t BQ25798_Ibus_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG31_IBUS_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}

uint16_t BQ25798_Ibat_read()
{
	uint16_t value;
    i2c_read16(&hi2c1, REG33_IBAT_ADC, &value, BQ25798_ADDR);
    return byteswap16(value);
}

uint16_t BQ25798_Sys_Min_Voltage_read()
{
	uint8_t value;
	uint8_t mask = 0x3F;
	uint16_t voltage;
    HAL_I2C_Mem_Read(&hi2c1, BQ25798_ADDR, REG00_Minimal_System_Voltage, I2C_MEMADD_SIZE_8BIT, &value, 1, 500);
    voltage= value & mask;
    voltage *= 250;
    voltage += 2500;
    return voltage;
}

uint16_t BQ25798_Chr_Volt_Limit_read()
{
	uint16_t value;
	i2c_read16(&hi2c1, REG01_Charge_Voltage_Limit, &value, BQ25798_ADDR);
    uint16_t swapvalue = byteswap16(value);
    return swapvalue * 10;
}

uint16_t BQ25798_Chr_Curr_Limit_read()
{
	uint16_t value;
	i2c_read16(&hi2c1, REG03_Charge_Current_Limit, &value, BQ25798_ADDR);
    uint16_t swapvalue = byteswap16(value);
    return swapvalue * 10;
}

uint8_t BQ25798_Sys_Min_Voltage_write(uint8_t bits) // 6 bits multiplier (2500mV + 6bits * 250mV) e.g 3000mV = 2500 + 3*250 = 3,25V / bits=3
{
	uint8_t res;
    res = i2c_write8(&hi2c1, REG00_Minimal_System_Voltage, (bits & 0x3F), BQ25798_ADDR);
    return res;
}

uint8_t BQ25798_Chr_Volt_Limit_write(uint16_t val)
{
	uint8_t res;
	val /= 10;
	res = i2c_write16(&hi2c1, REG01_Charge_Voltage_Limit, byteswap16(val), BQ25798_ADDR);
    return res;
}

uint8_t BQ25798_Chr_Curr_Limit_write(uint16_t val)
{
	uint8_t res;
	val /= 10;
	res = i2c_write16(&hi2c1, REG03_Charge_Current_Limit, byteswap16(val), BQ25798_ADDR);
    return res;
}

uint8_t BQ25798_Chrg_CTRL1_write(uint8_t vbus_bckp, uint8_t vac_ovp, uint8_t wdt_timer)
{
	uint8_t res, value;
	i2c_read8(&hi2c1, REG10_Charger_Control_1, &value, BQ25798_ADDR);
	printf("REG10: %x\r\n", value);
	printf("Binary\r\n");
	printbinary(value);
	printf("Binary MSB\r\n");
	printbinaryMSB(value);
//    res = i2c_write8(&hi2c1, REG00_Minimal_System_Voltage, (bits & 0xF7), BQ25798_ADDR);
    return res;
}
