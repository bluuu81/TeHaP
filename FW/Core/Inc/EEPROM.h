/*
 * EEPROM.h
 *
 *  Created on: Sep 19, 2023
 *      Author: Mariusz
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "stm32l4xx_hal.h"

#include "main.h"
#include <stdint.h>
#include <stdbool.h>



uint16_t Crc16_up(uint16_t crc, uint8_t data);
void Calc_config_crc(void);
uint8_t Load_config(void);
void Load_defaults();
void EEPROM_Print_config();
uint8_t Save_config(void);


#define BATT_ALARM_VOLTAGE	3300
#define CONFIG_VERSION  1


typedef struct {
  uint16_t 	version			:8;
  float		bat_scale;
  uint16_t	batt_alarm;
  uint16_t	reset_state		:1;
  uint16_t	disp_type		:3;
  uint16_t	tim_interval;
  uint16_t	TMP117_use		:1;
  uint16_t	TMP117_t_use	:1;
  uint16_t	TMP117_t_conf	:4;
  float		TMP117_t_offset;
  uint16_t	SHT3_use		:1;
  uint16_t	SHT3_t_use		:1;
  uint16_t	SHT3_t_conf		:4;
  uint16_t	SHT3_h_use		:1;
  uint16_t	SHT3_h_conf		:4;
  float		SHT3_t_offset;
  float		SHT3_h_offset;
  uint16_t	MS8607_use		:1;
  uint16_t	MS8607_t_use	:1;
  uint16_t	MS8607_t_conf	:4;
  uint16_t	MS8607_h_use	:1;
  uint16_t	MS8607_h_conf	:4;
  uint16_t	MS8607_p_use	:1;
  uint16_t	MS8607_p_conf	:4;
  float		MS8607_t_offset;
  float		MS8607_h_offset;
  float		MS8607_p_offset;
  uint16_t	BME280_use		:1;
  uint16_t	BME280_t_use	:1;
  uint16_t	BME280_t_conf	:4;
  uint16_t	BME280_h_use	:1;
  uint16_t	BME280_h_conf	:4;
  uint16_t	BME280_p_use	:1;
  uint16_t	BME280_p_conf	:4;
  float		BME280_t_offset;
  float		BME280_h_offset;
  float		BME280_p_offset;
  uint16_t	DPS368_use		:1;
  uint16_t	DPS368_t_use	:1;
  uint16_t	DPS368_t_conf	:4;
  uint16_t	DPS368_p_use	:1;
  uint16_t	DPS368_p_conf	:4;
  float		DPS368_t_offset;
  float		DPS368_p_offset;
  uint16_t 	checksum;
} __attribute__((packed))  Config_TypeDef;

extern Config_TypeDef config;
#endif /* INC_EEPROM_H_ */
