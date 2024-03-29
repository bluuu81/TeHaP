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

//clock strething ON
#define SHTC3_CMD_TEMP_HUM		0xA27C
#define SHTC3_CMD_TEMP_HUM_LP	0x5864
#define SHTC3_CMD_HUM_TEMP		0x245C
#define SHTC3_CMD_HUM_TEMP_LP	0xDE44

//clock strething OFF
//#define SHTC3_CMD_TEMP_HUM		0x6678
//#define SHTC3_CMD_TEMP_HUM_LP	0x9C60
//#define SHTC3_CMD_HUM_TEMP		0xE058
//#define SHTC3_CMD_HUM_TEMP_LP	0x1A40


enum SHTC3_mod	{normal = 0, lp = 1 };


typedef struct {
	uint8_t use_meas		:1;
	uint8_t anomaly			:1;
	uint8_t uu				:2;
	float	value;
	float	offset;
} __attribute__((packed)) MEAS_struct_t;


typedef struct {
	uint8_t present			:1;
	uint8_t sensor_use		:1;
	uint8_t sensor_conf		:4;
	MEAS_struct_t	temp;
	uint32_t meas_timestamp;
	uint8_t uu				:2;
} __attribute__((packed)) TMP117_struct_t;

typedef struct {
	uint8_t present			:1;
	uint8_t sensor_use		:1;
	uint8_t sensor_conf		:4;
	MEAS_struct_t	temp;
	MEAS_struct_t	hum;
	uint32_t meas_timestamp;
	uint8_t uu				:2;
} __attribute__((packed)) SHT3_struct_t;

typedef struct {
	uint8_t present			:1;
	uint8_t sensor_use		:1;
	uint8_t sensor_conf		:4;
	MEAS_struct_t	temp;
	MEAS_struct_t	hum;
	MEAS_struct_t	press;
	uint32_t meas_timestamp;
	uint8_t uu				:2;
} __attribute__((packed)) MS8607_struct_t;

typedef struct {
	uint8_t present			:1;
	uint8_t sensor_use		:1;
	uint8_t sensor_conf		:4;
	MEAS_struct_t	temp;
	MEAS_struct_t	hum;
	MEAS_struct_t	press;
	uint32_t meas_timestamp;
	uint8_t uu				:2;
} __attribute__((packed)) BME280_struct_t;

typedef struct {
	uint8_t present			:1;
	uint8_t sensor_use		:1;
	uint8_t sensor_conf		:4;
	MEAS_struct_t	temp;
	MEAS_struct_t	press;
	uint32_t meas_timestamp;
	uint8_t uu				:2;
} __attribute__((packed)) DPS368_struct_t;


void TCA9543A_SelectChannel(uint8_t channel);
void SET_BME280();
void SET_DPS368();

uint16_t byteswap16 (uint16_t bytes);
void printbinary(uint16_t value);
void printbinaryMSB(uint8_t value);
void setBit(uint8_t* reg, int bitNumber, int value);
void modifyRegister(unsigned char* reg, unsigned char mask, unsigned char value);
uint16_t combine_uint8(uint8_t high, uint8_t low);

void i2c_scan(I2C_HandleTypeDef * i2c, uint8_t addr_min, uint8_t addr_max);
uint8_t i2c_read8(I2C_HandleTypeDef * i2c, uint16_t offset, uint8_t *value, uint8_t addr);
uint8_t i2c_read16(I2C_HandleTypeDef * i2c, uint16_t offset, uint16_t *value, uint8_t addr);
uint8_t i2c_read24(I2C_HandleTypeDef * i2c, uint16_t offset, uint32_t *value, uint8_t addr);
uint8_t i2c_write8(I2C_HandleTypeDef * i2c, uint16_t offset, uint8_t value, uint8_t addr);
uint8_t i2c_write16(I2C_HandleTypeDef * i2c, uint16_t offset, uint16_t value, uint8_t addr);

uint8_t TMP117_check();
void TMP117_RST_Conf_Reg();
float TMP117_get_temp();
void TMP117_start_meas(uint8_t avg_mode);
uint16_t tmp117_avr_conf(uint8_t sensor_conf);

uint8_t MS8607_check();
float MS8607_get_temp();
float MS8607_get_press();
float MS8607_get_hum();

uint8_t SHTC3_check();
uint8_t SHTC3_start_meas(uint8_t mode);
uint8_t SHTC3_read_values(uint8_t* result);
float SHTC3_get_temp(uint8_t* result);
float SHTC3_get_hum(uint8_t* result);

uint8_t BME280_check();
void BME280_init_config(uint8_t conf_mode, uint8_t ovr_temp, uint8_t ovr_press, uint8_t ovr_hum, uint8_t coeff);
float BME280_get_temp();
float BME280_get_press();
float BME280_get_hum();
void BME280_start_meas();
void bme280_conf_change(uint8_t conf);

#endif /* INC_THP_SENSORS_H_ */
