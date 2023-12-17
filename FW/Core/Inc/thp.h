/*
 * thp.h
 *
 *  Created on: May 9, 2023
 *      Author: bluuu
 */

#ifndef INC_THP_H_
#define INC_THP_H_

#include "main.h"
#include "cli.h"
#include "thp_sensors.h"
#include "ctype.h"
#include <stdio.h>
#include <string.h>

#define HW_VER 10
#define FW_VER 2

extern ADC_HandleTypeDef hadc1;

extern CRC_HandleTypeDef hcrc;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;

//extern IWDG_HandleTypeDef hiwdg;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim16;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern TMP117_struct_t TMP117;
extern SHT3_struct_t SHT3;
extern MS8607_struct_t MS8607;
extern BME280_struct_t BME280;
extern DPS368_struct_t DPS368;

enum device_state
{
	INIT = 0,
	STANDBY,
	WORKING
};

void setLed2(uint8_t bri);
void led2Sweep(uint16_t spd, uint16_t cnt, uint16_t wait);

void check_powerOn();
void check_powerOff();
void MCUgoSleep();

void thp_loop();

uint8_t HALcalculateCRC(uint8_t data[], uint8_t len);
uint8_t calculateCRC(uint8_t data[], uint8_t len);

void display_values (uint8_t format);

#endif /* INC_THP_H_ */
