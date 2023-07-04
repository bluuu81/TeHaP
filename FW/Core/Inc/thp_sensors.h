/*
 * thp_sensors.h
 *
 *  Created on: 8 sty 2023
 *      Author: bluuu
 */

#include "main.h"
#include "bmp280.h"

#ifndef INC_THP_SENSORS_H_
#define INC_THP_SENSORS_H_

extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;

extern BMP280_HandleTypedef bmp280;

#define TCA9543A_ADDRESS 0x72 << 1

#define TMP117_ADDR 		0x49
#define TMP117_RESOLUTION	0.0078125f
#define TMP117_TEMP_REG		0x00
#define TMP117_CONF_REG		0x01
#define TMP117_ID_REG 		0x0F
#define TMP117_RESET_CONF	0x0220
#define TMP117_ID			0x1701 // 01117

enum TMP117_mod   { shutdown = 0x0400, one_shot = 0x0C00, continous = 0x0000 };
enum TMP117_avg   { no_avg = 0x0000, avg8 = 0x0020, avg32 = 0x0040, avg64 = 0x0060 };

#define SHTC3_ADDR_READ		(0x70 << 1) | 0x01
#define SHTC3_ADDR_WRITE	(0x70 << 1)

#define SHTC3_PRODUCT_CODE_MASK	0x083F
#define SHTC3_SENSOR_ID_MASK	0xF7C0

#define SHTC3_CMD_WAKEUP		0x1735
#define SHTC3_CMD_SLEEP			0x98B0
#define SHTC3_CMD_SOFT_RESET	0x5D80
#define SHTC3_CMD_READ_ID		0xC8EF
#define SHTC3_CMD_TEMP_HUM		0xA27C
#define SHTC3_CMD_TEMP_HUM_LP	0x5864
#define SHTC3_CMD_HUM_TEMP		0x245C
#define SHTC3_CMD_HUM_TEMP_LP	0xDE44


typedef struct {
	uint8_t sensor_present	:1;
	uint8_t temp_anomaly	:1;
	uint8_t sensor_conf		:4;
	uint8_t uu				:2;
	float	temperature;
	float	offset;
} __attribute__((packed)) TEMP_struct_t;

typedef struct {
	uint8_t sensor_present	:1;
	uint8_t press_anomaly	:1;
	uint8_t sensor_conf		:4;
	uint8_t uu				:2;
	float	pressure;
	float	offset;
} __attribute__((packed)) PRESS_struct_t;

typedef struct {
	uint8_t sensor_present	:1;
	uint8_t hum_anomaly	:1;
	uint8_t sensor_conf		:4;
	uint8_t uu				:2;
	float	humidity;
	float	offset;
} __attribute__((packed)) HUM_struct_t;


void TCA9543A_SelectChannel(uint8_t channel);
void SET_BME280();
void SET_DPS368();
void UNSET_BME_DPS();

void printbinary(uint16_t value);
void printbinaryMSB(uint8_t value);
void setBit(unsigned char* reg, int bitNumber, int value);
void modifyRegister(unsigned char* reg, unsigned char mask, unsigned char value);

void i2c_scan(I2C_HandleTypeDef * i2c, uint8_t addr_min, uint8_t addr_max);
uint8_t i2c_read8(I2C_HandleTypeDef * i2c, uint16_t offset, uint8_t *value, uint8_t addr);
uint8_t i2c_read16(I2C_HandleTypeDef * i2c, uint16_t offset, uint16_t *value, uint8_t addr);
uint8_t i2c_read24(I2C_HandleTypeDef * i2c, uint16_t offset, uint32_t *value, uint8_t addr);
uint8_t i2c_write8(I2C_HandleTypeDef * i2c, uint16_t offset, uint8_t value, uint8_t addr);
uint8_t i2c_write16(I2C_HandleTypeDef * i2c, uint16_t offset, uint16_t value, uint8_t addr);
uint8_t TMP117_check();
void TMP117_RST_Conf_Reg();
float TMP117_get_temp(uint8_t avg_mode);

uint8_t MS8607_check();
float MS8607_get_temp();
float MS8607_get_press();
float MS8607_get_hum();

uint8_t SHTC3_check();
float SHTC3_get_temp(uint8_t mode);
float SHTC3_get_hum(uint8_t mode);

uint8_t BME280_check();
void BME280_init_config(uint8_t conf_mode, uint8_t ovr_temp, uint8_t ovr_press, uint8_t ovr_hum, uint8_t coeff);
float BME280_get_temp();
float BME280_get_press();
float BME280_get_hum();

#endif /* INC_THP_SENSORS_H_ */
