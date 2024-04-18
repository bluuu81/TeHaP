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
#include "cmox_crypto.h"
#include "ctype.h"
#include <stdio.h>
#include <string.h>

#define HW_VER 10
#define FW_VER 9

#define GSM_RESTART_INTERVAL (30*3600*24)		// 30ms tick x secperhour x 24h (set to 0 for disable)
#define GPS_INTERVAL		 (60*60*12)			// w sekundach = 12h


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
extern uint16_t csvcnt;
extern volatile int seconds;

#define FNV_PRIME_32 16777619
#define FNV_BASIS_32 0x811C9DC5
#define SEC_PER_DAY	 (24*60*60)

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
void printCSVheader();
void getConfVars();
void ReinitTimer(uint16_t tim_interval);
void  GPRS_UserNewData(char *NewData, uint16_t len);
bool StartReadGps(void);
bool StartSendGPRS(void);
void SysTimeSync();

#endif /* INC_THP_H_ */
