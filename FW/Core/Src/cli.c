/*
 * cli.c
 *
 *  Created on: May 16, 2023
 *      Author: bluuu
 */

#include "cli.h"
#include "adc.h"
#include "main.h"
#include "thp_sensors.h"
#include "thp.h"
#include "bq25798.h"
#include "ctype.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "EEPROM.h"


#define DEBUG_BUF_SIZE 512
#define SIM_BUF_SIZE 512

extern uint8_t charger_state;
extern Config_TypeDef config;

extern uint16_t tim_interval;
extern uint16_t new_tim_interval;

uint8_t  debug_rx_buf[DEBUG_BUF_SIZE];
uint16_t debug_rxtail;

uint8_t sim_rx_buf[32];    // 32 bytes buffer
uint16_t sim_rxtail;

uint32_t temp;


static char clibuf[32];
static int cliptr;

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, len+1);  // uart1
    return len;
}

void debug_putchar(uint8_t ch)
{
    HAL_UART_Transmit(&huart1, &ch, 1, 2);  // debug uart
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart1) HAL_UART_Receive_IT(&huart1, debug_rx_buf, DEBUG_BUF_SIZE);  // Interrupt start Uart1 RX
	if(huart == &huart2) HAL_UART_Receive_IT(&huart2, sim_rx_buf, SIM_BUF_SIZE); // Interrupt start Uart2 RX
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    HAL_UART_RxCpltCallback(huart);
}

uint16_t UART_has_char()      // Return number of bytes in buffer
{
    return (huart1.RxXferSize-huart1.RxXferCount+DEBUG_BUF_SIZE-debug_rxtail) % DEBUG_BUF_SIZE;
}

uint8_t UART_receive()       // Receive byte from buffer
{
    uint8_t tmp = debug_rx_buf[debug_rxtail++];
    if(debug_rxtail >= DEBUG_BUF_SIZE) debug_rxtail = 0;
    return tmp;
}

// string functions

char * find(const char *arg2)							// find token in cmdline
{
	int i,j,k;
	for(i=0; clibuf[i]; i++)
		for(j=i, k=0; tolower(clibuf[j]) == arg2[k]; j++, k++)
			if(!arg2[k+1]) return (clibuf+(j+1));
	return NULL;
}

char * getval(char *p, int32_t *val, int32_t min, int32_t max)		// get s32 value from cmdline
{
	int32_t tmp = 0;
	if(*p == ' ') p++;
	uint8_t sign = (*p == '-') ? 1:0;
	while(*p)
	{
		if(*p >= '0' && *p <= '9') {tmp *= 10; tmp += *p - '0'; p++;}
		else break;
	}
	if(sign) tmp = -tmp;
	if(tmp >= min && tmp <= max) *val = tmp; else printf("Bad value\r\n");
	return p;
}

void getString(char *p, char *dst, int16_t minlen, int16_t maxlen, const char *nam)		// get string from cmdline
{
	if(*p == ' ') p++;
	for(int i=0; i<maxlen; i++) if(p[i]==13 && i<minlen) {printf("Too short\r\n"); return;}	// test dlugosci
	for(int i=0; i<maxlen; i++)
	{
		dst[i] = p[i];
		if(p[i] == 13) {dst[i] = 0; break;}
	}
	printf("%s: %s\r\n", nam, dst);
}

char * getFloat (char *p, float *val, float min, float max)
{
	 char* pend;
	float tmp = 0;
		while(*p == ' ') p++;
		tmp = strtof(p, &pend);
		if(tmp >= min && tmp <= max) {*val = tmp; return 1;} else { printf("Bad value\r\n"); return 0;}

}
char * get_hex(char *p, uint8_t chars, uint16_t *val)
{
    uint16_t tmp = 0;
    if(*p==' ' || *p==',') p++;
    while(*p)
    {
        if(*p == 13 || *p == 10) {p = NULL; break;}
        if(*p==' ' || *p==',') break;
        char asc = tolower(*p);
        if(asc >= '0' && asc <= '9')      {tmp *= 16; tmp += asc - '0'; p++;}
        else if(asc >= 'a' && asc <= 'f') {tmp *= 16; tmp += asc - 'a'+10; p++;}
        else p++;
        if(--chars == 0) break;
    }
    *val = tmp;
    return p;
}

void CLI() {
    int len = UART_has_char();
    if(len) { for(int i=0; i<len; ++i) CLI_proc(UART_receive()); }
}

void CLI_proc(char ch)
{
	char *p;

	float tempfloat;
	if(cliptr < sizeof(clibuf)) clibuf[cliptr++] = ch;
	if(ch == 10)	// LF
	{
	    if(clibuf[cliptr-1] == 13) cliptr--;
		memset(clibuf+cliptr, 0, sizeof(clibuf)-cliptr);
		cliptr = 0;
// Main commands ------------------------------------------------------------------------------
		if(find("?")==clibuf+1 || find("help")==clibuf+4)	{help(); return;}
		if(find("i2cscan")==clibuf+7) {i2c_scan(&hi2c2, 0x38, 0xA0); return;}
		if(find("clearconfig")==clibuf+11) {printf("config reset to defaults"); Load_defaults(); return;}
		if(find("printconfig")==clibuf+11) {EEPROM_Print_config(); return;}
		if(find("loadconfig")==clibuf+10) {printf("LOADING CONFIG. Status: %i (0==OK)\r\n",Load_config()); return;}
		if(find("saveconfig")==clibuf+10) {printf("SAVING CONFIG. Status: %i (0==NO CHANGES; 1==SAVE OK, 2==ERR)\r\n",Save_config()); return;}
		if(find("setbattalarm")==clibuf+12){getval(clibuf+13, &temp, 0, 15000); config.batt_alarm=temp; printf("Batt alarm:%i",config.batt_alarm); return;};
		if(find("setbatscale")==clibuf+11){getFloat(clibuf+12, &tempfloat, -10.0, 10.0); config.bat_scale=tempfloat; printf("Batt scale:%f \r\n",config.bat_scale); return;};
//		if(find("setoffset")==clibuf+9){setOffset();return;}

		p = find("set ");
		if(p == clibuf+4)
		{
			if(p = find("interval "))
			{
				int32_t tmp = -1;
	            getval(p, &tmp, 4, 3600);
		            if(tmp >= 4)
		            {
		                new_tim_interval = tmp;
		                printf("New meas interval: %u\r\n", new_tim_interval);
		            }
		            return;
			}
		}
	}
//		if(find("load defaults")==clibuf+13)
//		{
//			Load_defaults();
//			Save_config();
//			printf("return to defaults ...\r\n");
//			return;
//		}
//// ................................................................................
//        if((p = find("debug ")))
//        {
//            int32_t tmp = -1;
//            getval(p, &tmp, 0, 2);
//            if(tmp >= 0)
//            {
//                debug_level = tmp;
//                printf("Debug: %u\r\n", debug_level);
//            }
//            return;
//        }
//
//        p = find("charger ");           // set commands
//        if(p == clibuf+8)
//        {
//            if((p = find("start")))
//            {
//            	printf("Start charging\r\n");
//            	start_charging();
//                return;
//            }
//            if((p = find("stop")))
//            {
//            	printf("Stop charging\r\n");
//            	stop_charging();
//                return;
//            }
//
//        }
//	}
}


/*void setOffset(void)
{ float valtostore;

switch (clibuf[10])
{

case 'h':
	if(find("sht3")==clibuf+16)
	{
		if (getFloat(clibuf+17, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.SHT3_h_offset=valtostore;}
		printf("SHT3 hum offset:%f \r\n",config.SHT3_h_offset);
		return;
		break;
	}

	if(find("ms8607")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.MS8607_h_offset=valtostore;}
		printf("MS8607 hump offset:%f \r\n",config.MS8607_h_offset);
		return;
		break;
	}
	if(find("bme280")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.BME280_h_offset=valtostore;}
		printf("BME280 hum offset:%f \r\n",config.BME280_h_offset);
		return;
		break;
	}

	printf("unknown sensor");
	break;
case 'p':
	if(find("ms8607")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.MS8607_p_offset=valtostore;}
		printf("MS8607 press offset:%f \r\n",config.MS8607_p_offset);
		return;
		break;
	}
	if(find("bme280")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.BME280_p_offset=valtostore;}
		printf("BME280 press offset:%f \r\n",config.BME280_p_offset);
		return;
		break;
	}
	if(find("dps368")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.DPS368_p_offset=valtostore;}
		printf("DPS368 press offset:%f \r\n",config.DPS368_p_offset);
		return;
		break;
	}
	printf("unknown sensor");
	break;
case 't':
	if(find("tmp117")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.TMP117_t_offset=valtostore;}
		printf("TMP117 temp offset:%f \r\n",config.TMP117_t_offset);
		return;
		break;
	}

	if(find("sht3")==clibuf+16)
	{
		if (getFloat(clibuf+17, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.SHT3_t_offset=valtostore;}
		printf("SHT3 temp offset:%f \r\n",config.SHT3_t_offset);
		return;
		break;
	}

	if(find("ms8607")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.MS8607_t_offset=valtostore;}
		printf("MS8607 temp offset:%f \r\n",config.MS8607_t_offset);
		return;
		break;
	}
	if(find("bme280")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.BME280_t_offset=valtostore;}
		printf("BME280 temp offset:%f \r\n",config.BME280_t_offset);
		return;
		break;
	}
	if(find("dps368")==clibuf+18)
	{
		if (getFloat(clibuf+19, &valtostore, MIN_OFFSET, MAX_OFFSET))
		{config.DPS368_t_offset=valtostore;}
		printf("DPS368 temp offset:%f \r\n",config.DPS368_t_offset);
		return;
		break;
	}
	printf("unknown sensor");
	break;

default:
	printf( "unknown parameter");
	break;
}

printf("bad parameters. usage: setoffset X YYYY ff.fff | x:t/p/h | Y:sensor name | ff.fff: offset\r\n");
return;
}*/



void help()
{
	printf("--- THP HW v%1.1f  FW v%1.1f --- \r\n", HW_VER*0.1f,FW_VER*0.1f );
	printf("Charger state : ");
	switch (charger_state)
	{
	case 0:
		printf("FAULT\r\n");
		break;
	case 1:
		printf("OK\r\n");
		break;
	case 2:
		printf("No charging ...\r\n");
		break;
	case 3:
		printf("Charging ...\r\n");
		break;
	}
	printf("MCU Temp: %3.1f [degC]\r\n", GET_MCU_Temp());
	printf("VBAT: %u [mV]  ", BQ25798_Vbat_read());
	printf("Vac1: %u [mV]  ", BQ25798_Vac1_read());
	printf("Vac2: %u [mV]  ", BQ25798_Vac2_read());
	printf("VSYS: %u [mV]  \r\n", BQ25798_Vsys_read());
	printf("Ibus: %u [mA]  ", BQ25798_Ibus_read());
	printf("Ibat: %u [mA]  \r\n", BQ25798_Ibat_read());
	printf("Minimal SYS Voltage: %u [mV]  \r\n", BQ25798_Sys_Min_Voltage_read());
	printf("Charge Voltage Limit: %u [mV]  \r\n",BQ25798_Chr_Volt_Limit_read());
	printf("Charge Current Limit: %u [mA]  \r\n",BQ25798_Chr_Curr_Limit_read());
	BQ25798_Chrg_CTRL3_read();
	BQ25798_Chrg_CTRL4_read();
	BQ25798_Chrg_FAULT1_read();
	BQ25798_Chrg_FAULT2_read();
	BQ25798_Chrg_STAT0_read();
	BQ25798_Chrg_STAT1_read();
	BQ25798_Chrg_STAT2_read();
	BQ25798_Chrg_STAT3_read();
	BQ25798_Chrg_STAT4_read();
	printf("-----------------\r\n");

}
;
