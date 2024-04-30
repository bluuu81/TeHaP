/*
 * thp.h
 *
 *  Created on: May 9, 2023
 *      Author: bluuu
 */

#ifndef INC_THP_H_
#define INC_THP_H_

#include "Sim80xDrv.h"
#include "main.h"
#include "cli.h"
#include "thp_sensors.h"
#include "cmox_crypto.h"
#include "ctype.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


#define HW_VER 10
#define FW_VER 10

#define GSM_RESTART_INTERVAL (30*3600*24)		// 30ms tick x secperhour x 24h (set to 0 for disable)
#define GPS_INTERVAL		 (60*60*12)			// w sekundach = 12h

#define SECS_PER_MIN  ((uint32_t)(60UL))
#define SECS_PER_HOUR ((uint32_t)(3600UL))
#define SECS_PER_DAY  ((uint32_t)(SECS_PER_HOUR * 24UL))
#define LEAP_YEAR(Y)  ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

#define GPRS_TOKEN	0xDEF8

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

enum GPRS_send_types
{
	STATUS = 20,
	LOCALIZE,
	TEMP,
	PRESS,
	HUM
};

typedef struct
{
	uint8_t send_type;
	float 	sensor1_val;
	float 	sensor2_val;
	float 	sensor3_val;
	uint8_t anomaly1	:1;
	uint8_t anomaly2	:1;
	uint8_t anomaly3	:1;
	uint8_t	uu			:5;
} __attribute__((__packed__)) Meas_Send_t;  //14B

typedef struct
{
	uint16_t	token;
	uint32_t	UID;
	uint8_t		send_type;
	float		MCU_temp;
	uint16_t	Vbat;
	uint16_t	Vac1;
	uint16_t	Vac2;
	uint8_t		charg_state;
	uint32_t	timestamp;
	uint8_t		spare[9];
	uint8_t		crc;
} __attribute__((__packed__)) GPRS_status_t;

typedef struct
{
	uint16_t	token;
	uint32_t	UID;
	uint8_t		send_type;
	float		lat;
	float		lon;
	uint8_t		sats;
	uint8_t		fix;
	uint32_t	timestamp;
	uint8_t		spare[10];
	uint8_t		crc;
} __attribute__((__packed__)) GPRS_localize_t;

typedef struct
{
	uint16_t	token;
	uint32_t	UID;
	Meas_Send_t meas_frame;
	uint32_t	timestamp;
	uint8_t		spare[7];
	uint8_t		crc;
} __attribute__((__packed__)) GPRS_meas_frame_t;


void setLed2(uint8_t bri);
void led2Sweep(uint16_t spd, uint16_t cnt, uint16_t wait);

void check_powerOn();
void check_powerOff();
void MCUgoSleep();

void thp_loop();

uint8_t HALcalculateCRC(uint8_t data[], uint8_t len);
//uint8_t calculateCRC(uint8_t data[], uint8_t len);

void display_values (uint8_t format);
void printCSVheader();
void getConfVars();
void ReinitTimer(uint16_t tim_interval);
void  GPRS_UserNewData(char *NewData, uint16_t len);
bool StartReadGps(void);
bool StartSendGPRS(void);
void SysTimeSync();
//uint32_t time_to_unix(Sim80x_Time_t *tm);

#endif /* INC_THP_H_ */
