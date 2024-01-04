/*
 * thp_sensors.c
 *
 *  Created on: 8 sty 2023
 *      Author: bluuu
 */
#include "thp_sensors.h"
#include "ms8607.h"
#include "ms8607_i2c.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



uint8_t i2c_read8(I2C_HandleTypeDef * i2c, uint16_t offset, uint8_t *value, uint8_t addr)
{
	uint8_t tmp;
    uint8_t res = HAL_I2C_Mem_Read(i2c, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 1, 8);
    *value = tmp;
    return res;
}

uint8_t i2c_read16(I2C_HandleTypeDef * i2c, uint16_t offset, uint16_t *value, uint8_t addr)
{
	uint16_t tmp;
    uint8_t res = HAL_I2C_Mem_Read(i2c, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 2, 8);
    *value = tmp;
    return res;
}

uint8_t i2c_read16_int(I2C_HandleTypeDef * i2c, uint16_t offset, int16_t *value, uint8_t addr)
{
	uint16_t tmp;
    uint8_t res = HAL_I2C_Mem_Read(i2c, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 2, 8);
    *value = tmp;
    return res;
}

uint8_t i2c_read24(I2C_HandleTypeDef * i2c, uint16_t offset, uint32_t *value, uint8_t addr)
{
	uint32_t tmp;
    uint8_t res = HAL_I2C_Mem_Read(i2c, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 3, 8);
    *value = tmp;
    return res;
}

uint8_t i2c_write8(I2C_HandleTypeDef * i2c, uint16_t offset, uint8_t value, uint8_t addr)
{
	uint8_t tmp = value;
    uint8_t res = HAL_I2C_Mem_Write(i2c, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 1, 8);
    return res;
}

uint8_t i2c_write16(I2C_HandleTypeDef * i2c, uint16_t offset, uint16_t value, uint8_t addr)
{
	uint16_t tmp = value;
    uint8_t res = HAL_I2C_Mem_Write(i2c, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 2, 8);
    return res;
}

void clr_bit16(I2C_HandleTypeDef * i2c, unsigned char sub_address, unsigned short new_word, uint8_t addr) {
    unsigned short old_word;
    i2c_read16(i2c, sub_address, &old_word, addr);
    old_word &= ~new_word;
    i2c_write16(i2c, sub_address, old_word, addr);
}

void set_bit16(I2C_HandleTypeDef * i2c, unsigned char sub_address, unsigned short new_word, uint8_t addr)
{
    unsigned short old_word;
    i2c_read16(i2c, sub_address, &old_word, addr);
    old_word |= new_word;
    i2c_write16(i2c, sub_address, old_word, addr);
}

void setBit(uint8_t* reg, int bitNumber, int value) {
    if (value == 0) {
        *reg &= ~(1 << bitNumber);  // Ustawienie bitu na 0
    } else if (value == 1) {
        *reg |= (1 << bitNumber);   // Ustawienie bitu na 1
    }
}

void modifyRegister(unsigned char* reg, unsigned char mask, unsigned char value) {
    *reg = (*reg & ~mask) | (value & mask);
}

void i2c_scan(I2C_HandleTypeDef * i2c, uint8_t addr_min, uint8_t addr_max)
{
	printf("Scanning I2C devices ...\r\n");
	for (uint8_t addr = addr_min; addr <= addr_max; addr++)
	{
		HAL_StatusTypeDef status;
		status = HAL_I2C_IsDeviceReady(i2c, addr << 1, 3, 500);
		HAL_Delay(100);
		if (status == HAL_OK) {
			    	printf("Device found on %#x \r\n", addr);
			    } else {
			    	printf("Device NOTfound on %#x \r\n", addr);
			    	HAL_Delay(100);
			    }
	}
}

uint16_t byteswap16 (uint16_t bytes)
{
//	return ((bytes & 0xFF) << 8) | ((bytes >> 8) & 0xFF);
	return __REV16(bytes);
}

void printbinary(uint16_t value)
{
	while (value) {
		if (value & 1)
			printf("1");
		else
			printf("0");
		value >>= 1;
	}
	printf("\r\n");
}

void printbinaryMSB(uint8_t value)
{
  for (int i = 7; i >= 0; i--) {
    if (value & (1 << i))
      printf("1");
    else
      printf("0");
  }
  printf("\r\n");
}

void TCA9543A_SelectChannel(uint8_t channel)
{
if (channel >1) {
	printf("Wrong parameter\r\n");
	return;
}
if (channel == 0 || channel == 1) {
		I2C2TCA_NRST();
		HAL_Delay(1);
		uint8_t data = 1 << channel;
		HAL_I2C_Mem_Write(&hi2c2, TCA9543A_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
	}
}

void SET_BME280()
{
	TCA9543A_SelectChannel(0);
}

void SET_DPS368()
{
	TCA9543A_SelectChannel(1);
}



uint8_t TMP117_check()
{
	HAL_StatusTypeDef status;
	status = HAL_I2C_IsDeviceReady(&hi2c2, TMP117_ADDR << 1, 3, 500);
	HAL_Delay(100);
	if (status == HAL_OK) {
		uint16_t value;
		i2c_read16(&hi2c2, TMP117_ID_REG, &value, TMP117_ADDR << 1);
		if(value == TMP117_ID) {printf("TMP117 OK\r\n"); return 1;} else {printf("NOT TMP117\r\n"); return 0;}
	} else {printf("TMP117 FAILED\r\n"); return 0;}
}

void TMP117_RST_Conf_Reg()
{
//	printf("TMP117 RESET REG\r\n");
	i2c_write16(&hi2c2, TMP117_CONF_REG, TMP117_RESET_CONF, TMP117_ADDR << 1);
	HAL_Delay(1);

}

float TMP117_get_temp()
{
	int16_t value;
    i2c_read16_int(&hi2c2, TMP117_TEMP_REG, &value, TMP117_ADDR << 1);
    return (float)byteswap16(value) * TMP117_RESOLUTION;
}

void TMP117_start_meas(uint8_t avg_mode)
{
	uint16_t config, swapconfig;
	TMP117_RST_Conf_Reg();
	i2c_read16(&hi2c2, TMP117_CONF_REG, &config, TMP117_ADDR << 1);
	swapconfig = byteswap16(config);
//	printf("REG in TMP (hex): %x \r\n", swapconfig);
	swapconfig |= avg_mode;
	swapconfig |= one_shot;
	config = byteswap16(swapconfig);
	i2c_write16(&hi2c2, TMP117_CONF_REG, config, TMP117_ADDR << 1);
	HAL_Delay(2);
}

uint16_t tmp117_avr_conf(uint8_t sensor_conf)
{
    switch (sensor_conf) {
        case 0:
        	printf("TMP117 set no_avg\r\n");
            return no_avg;
        case 1:
        	printf("TMP117 set avg8\r\n");
            return avg8;
        case 2:
        	printf("TMP117 set avg32\r\n");
            return avg32;
        case 3:
        	printf("TMP117 set avg64\r\n");
            return avg64;
        default:
        	printf("TMP117 set no_avg\r\n");
            return no_avg;
    }
}

uint8_t MS8607_check()
{
	ms8607_init();
	if(ms8607_is_connected()) {printf("MS8607 OK\r\n"); return 1;
	} else {printf("MS8607 FAIL\r\n"); return 0;};
	ms8607_reset();
}


float MS8607_get_temp()
{
	float temp;
	ms8607_read_temperature(&temp);
//	printf("MS Temp: %f\r\n",temp);
	return temp;
}

float MS8607_get_press()
{
	float press;
	ms8607_read_pressure(&press);
//	printf("MS Press: %f\r\n",press);
	return press;
}

float MS8607_get_hum()
{
	float hum;
	ms8607_read_humidity(&hum);
//	printf("MS Hum: %f\r\n",hum);
	return hum;
}

uint8_t SHTC3_wakeup()
{
	HAL_StatusTypeDef status;
	uint16_t command = SHTC3_CMD_WAKEUP;
	status = HAL_I2C_Master_Transmit(&hi2c2, SHTC3_ADDR_WRITE, (uint8_t*)&command, 2, 150);
//	status = HAL_I2C_Mem_Write(&hi2c2, SHTC3_ADDR_WRITE, 0x00, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&command, 1, HAL_MAX_DELAY);
//	HAL_Delay(13);
	if(status == HAL_OK) { return 1; }
	else {printf("SHTC3 Wake up fail %x\r\n", status); return 0; }
}

uint8_t SHTC3_sleep()
{
	HAL_StatusTypeDef status;
	uint16_t command = SHTC3_CMD_SLEEP;
	status = HAL_I2C_Master_Transmit(&hi2c2, SHTC3_ADDR_WRITE, (uint8_t*)&command, 2, 150);
	HAL_Delay(2);
	if(status == HAL_OK) return 1;
	else return 0;
}


uint8_t SHTC3_check()
{
	HAL_StatusTypeDef status, status2;
	uint8_t data[2];
	status = HAL_I2C_IsDeviceReady(&hi2c2, SHTC3_ADDR_WRITE, 3, 500);
	HAL_Delay(2);
	if (status == HAL_OK) {
		SHTC3_wakeup();
		uint16_t command = SHTC3_CMD_READ_ID;
		status2 = HAL_I2C_Master_Transmit(&hi2c2, SHTC3_ADDR_WRITE, (uint8_t*)&command, 2, 150);
		status2 = HAL_I2C_Master_Receive(&hi2c2, SHTC3_ADDR_READ, (uint8_t*)data, 2, 150);
		  if (status2 == HAL_OK) {
			  uint16_t id = data[0] << 8 | data[1];
			  uint16_t code = id & SHTC3_PRODUCT_CODE_MASK;
			  if (code == 0x807) {
				  printf("SHTC3 OK\r\n");
//				  SHTC3_start_meas(0);
//				  SHTC3_sleep();
				  return 1;
			  }
		  } else {printf("NO SHTC3\r\n"); return 0;}
	} else {printf("SHTC3 FAILED\r\n"); return 0;}
	return 0;
}

uint8_t SHTC3_start_meas(uint8_t mode)
{
	HAL_StatusTypeDef status;
	uint16_t command;
	SHTC3_wakeup();
	if(mode == 0) command = SHTC3_CMD_TEMP_HUM;
	else command = SHTC3_CMD_TEMP_HUM_LP;
	status = HAL_I2C_Master_Transmit(&hi2c2, SHTC3_ADDR_WRITE, (uint8_t*)&command, 2, 150);
	return status; //0 = OK
}

uint8_t SHTC3_read_values(uint8_t* result)
{
	HAL_StatusTypeDef status;
	status = HAL_I2C_Master_Receive(&hi2c2, SHTC3_ADDR_READ, (uint8_t*)result, 6, 500);
//	HAL_Delay(20);
	if (status != HAL_OK) {
		return 0;
	} 	else {
		return 1;
	};
}

float SHTC3_get_temp(uint8_t* result)
{
	uint16_t raw_temp = result[0] << 8 | result[1];
	uint8_t data[2] = {raw_temp >> 8, raw_temp & 0xFF};
	uint8_t crc_hal = HALcalculateCRC(data,2);
	if(result[2] == crc_hal) {
		return (float)(((raw_temp * 175.0f) / 65535.0f) - 45.0f);
	}
	else {printf("Bad CRC\r\n");};
	return -1000.0;
}

float SHTC3_get_hum(uint8_t* result)
{
	uint16_t raw_hum = result[3] << 8 | result[4];
	uint8_t data[2] = {raw_hum >> 8, raw_hum & 0xFF};
	uint8_t crc_hal = HALcalculateCRC(data,2);
	if(result[5] == crc_hal) {
		return (float)((raw_hum * 100.0f) / 65535.0f);
	}
	else {printf("Bad CRC\r\n");};
	return -1000.0;
}

uint8_t BME280_check()
{
	uint8_t value;
	SET_BME280();
	HAL_StatusTypeDef status;
	status = HAL_I2C_IsDeviceReady(&hi2c2, BMP280_I2C_ADDRESS_1 << 1, 3, 150);
	HAL_Delay(100);
	if (status == HAL_OK) {
		i2c_read8(&hi2c2, BMP280_REG_ID, &value, BMP280_I2C_ADDRESS_1 << 1);
		if(value == BME280_CHIP_ID) {printf("BME280 OK\r\n"); return 1;} else {printf("NOT BME280\r\n"); return 0;}
	} else {printf("BME280 FAILED\r\n"); return 0;}
	return 0;

}

void BME280_init_config(uint8_t conf_mode, uint8_t ovr_temp, uint8_t ovr_press, uint8_t ovr_hum, uint8_t coeff)
{
	SET_BME280();
//	bmp280_init_default_params(&bmp280.params);

	bmp280.params.filter = coeff;
	bmp280.params.oversampling_pressure = ovr_press;
	bmp280.params.oversampling_temperature = ovr_temp;
	bmp280.params.oversampling_humidity = ovr_hum;
//	bmp280.params.standby = BMP280_STANDBY_250;
	bmp280.addr = BMP280_I2C_ADDRESS_1;
	bmp280.i2c = &hi2c2;

	switch (conf_mode)
	{
	case 1:
		bmp280.params.mode = BMP280_MODE_FORCED;
		break;
	case 2:
		bmp280.params.mode = BMP280_MODE_NORMAL;
		break;
	default:
		bmp280.params.mode = BMP280_MODE_NORMAL;
	  }
	if(bmp280_init(&bmp280, &bmp280.params)) printf("BME280 init OK\r\n"); else printf("BME280 init FAIL\r\n");
}

float BME280_get_temp()
{
	SET_BME280();
	float temp, press, hum;
	while(bmp280_is_measuring(&bmp280));
	bmp280_read_float(&bmp280, &temp, &press, &hum);
	return temp;
}

float BME280_get_press()
{
	SET_BME280();
	float temp, press, hum;
	while(bmp280_is_measuring(&bmp280));
	bmp280_read_float(&bmp280, &temp, &press, &hum);
	return press;
}

float BME280_get_hum()
{
	SET_BME280();
	float temp, press, hum;
	while(bmp280_is_measuring(&bmp280));
	bmp280_read_float(&bmp280, &temp, &press, &hum);
	return hum;
}

void BME280_start_meas()
{
	SET_BME280();
	if(!bmp280_force_measurement(&bmp280)) printf("Komenda w BME280 niewykonana\r\n");
}
