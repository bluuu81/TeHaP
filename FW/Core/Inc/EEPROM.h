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
uint8_t EEPROM_Load_config(void);


#define BATT_ALARM_VOLTAGE	3300
#define CONFIG_VERSION  1


typedef struct {
  uint16_t version;
  float	bat_scale;
  uint16_t	batt_alarm;
  float	TMP117_t_offset;
  float	SHT3_t_offset;
  float	SHT3_h_offset;
  float	MS8607_t_offset;
  float	MS8607_h_offset;
  float	MS8607_p_offset;
  float	BME280_t_offset;
  float	BME280_h_offset;
  float	BME280_p_offset;
  float	DPS368_t_offset;
  float	DPS368_p_offset;
  uint8_t	TMP117_config;
  uint8_t SHT3_config;
  uint8_t DPS368_config;
  uint8_t MS8607_config;
  uint16_t checksum;
} __attribute__((packed))  Config_TypeDef;

extern Config_TypeDef config;
#endif /* INC_EEPROM_H_ */
