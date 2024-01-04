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
extern uint8_t disp_type;
extern uint16_t meas_count;
extern uint8_t meas_cont_mode;
extern uint16_t tmp117_avr;

uint16_t csvcnt;

uint8_t  debug_rx_buf[DEBUG_BUF_SIZE];
uint16_t debug_rxtail;

uint8_t sim_rx_buf[32];    // 32 bytes buffer
uint16_t sim_rxtail;

static char clibuf[64];
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
		if(tmp >= min && tmp <= max) {*val = tmp; } else { printf("Bad value\r\n"); }
		return p;
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
	int32_t temp;
	float tempfloat;
	if(cliptr < sizeof(clibuf)) clibuf[cliptr++] = ch;
	if(ch == 10)	// LF
	{
	    if(clibuf[cliptr-1] == 13) cliptr--;
		memset(clibuf+cliptr, 0, sizeof(clibuf)-cliptr);
		cliptr = 0;
// Main commands ------------------------------------------------------------------------------
		if(find("?")==clibuf+1 || find("help")==clibuf+4)	{print_help(); return;}
		if(find("i2cscan")==clibuf+7) {i2c_scan(&hi2c2, 0x38, 0xA0); return;}
		if(find("clearconfig")==clibuf+11) {printf("config reset to defaults\r\n"); Load_defaults(); return;}
		if(find("printconfig")==clibuf+11) {EEPROM_Print_config(); return;}
		if(find("loadconfig")==clibuf+10) {printf("LOADING CONFIG. Status: %i (0==OK)\r\n",Load_config()); return;}
		if(find("saveconfig")==clibuf+10) {printf("SAVING CONFIG. Status: %i (0==NO CHANGES; 1==SAVE OK, 2==ERR)\r\n",Save_config()); return;}
		if(find("setbattalarm")==clibuf+12){getval(clibuf+13, &temp, 0, 15000); config.batt_alarm=temp; printf("Batt alarm:%i",config.batt_alarm); return;};
		if(find("setbatscale")==clibuf+11){getFloat(clibuf+12, &tempfloat, -10.0, 10.0); config.bat_scale=tempfloat; printf("Batt scale:%f \r\n",config.bat_scale); return;};

		p = find("set ");
		if(p == clibuf+4)
		{
			if((p = find("interval ")))
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
			if((p = find("disptype ")))
			{
				int32_t tmp = -1;
	            getval(p, &tmp, 0, 2);
		            if(tmp >= 1)
		            {
		            	if(tmp==1) printf("Display type TXT\r\n"); else if(tmp==2) { printf("Display type CSV"); printCSVheader();}
		            	csvcnt = 0;
		            	disp_type = tmp;
		            }
		            else {disp_type = tmp; printf("Silent mode\r\n");}
		            return;
			}
			if((p = find("tmp117 ")))
			{
				if(p == clibuf+11)
				{
					if((p = find("enable")))
					{
						config.TMP117_use = 1;
						TMP117.sensor_use = 1;
						printf("TMP117 sensor enabled\r\n");
						Save_config();
					}
					if((p = find("disable")))
					{
						config.TMP117_use = 0;
						TMP117.sensor_use = 0;
						printf("TMP117 sensor disabled\r\n");
						Save_config();
					}
					if((p = find("temperature ")))
					{
						if(p == clibuf+23)
						{
							if((strstr(clibuf+23, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+30, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.TMP117_t_offset = tmp;
					            TMP117.temp.offset = tmp;
					            printf("TMP117 temperature offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+23, "conf ")))
							{
								int32_t tmp = -1;
					            getval(clibuf+28, &tmp, 0, 3);
					            config.TMP117_t_conf = tmp;
					            TMP117.temp.sensor_conf = tmp;
					            tmp117_avr=tmp117_avr_conf(TMP117.temp.sensor_conf);
					            printf("TMP117 temperature config %li\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+23, "en")))
							{
								config.TMP117_t_use = 1;
								TMP117.temp.use_meas = 1;
								printf("TMP117 temperature measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+23, "dis")))
							{
								config.TMP117_t_use = 0;
								TMP117.temp.use_meas = 0;
								printf("TMP117 temperature measure disable\r\n");
								Save_config();
							}
						return;
						}
					}
				}
			}
			if((p = find("shtc3 ")))
			{
				if(p == clibuf+10)
				{
					if((p = find("enable")))
					{
						config.SHT3_use = 1;
						SHT3.sensor_use = 1;
						printf("SHTC3 sensor enabled\r\n");
						Save_config();
					}
					if((p = find("disable")))
					{
						config.SHT3_use = 0;
						SHT3.sensor_use = 0;
						printf("SHTC3 sensor disabled\r\n");
						Save_config();
					}
					if((p = find("temperature ")))
					{
						if(p == clibuf+22)
						{
							if((strstr(clibuf+22, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+29, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.SHT3_t_offset = tmp;
					            SHT3.temp.offset = tmp;
					            printf("SHTC3 temperature offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+22, "en")))
							{
								config.SHT3_t_use = 1;
								SHT3.temp.use_meas = 1;
								printf("SHTC3 temperature measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+22, "dis")))
							{
								config.SHT3_t_use = 0;
								SHT3.temp.use_meas = 0;
								printf("SHTC3 temperature measure disable\r\n");
								Save_config();
							}
						}
					}
					if((p = find("hum ")))
					{
						if(p == clibuf+14)
						{
							if((strstr(clibuf+14, "offset ")))
							{
								float tmp;
						        getFloat(clibuf+21, &tmp, MIN_OFFSET, MAX_OFFSET);
						        config.SHT3_h_offset = tmp;
						        SHT3.hum.offset = tmp;
						        printf("SHTC3 humidity offset %.6f\r\n",tmp);
						        Save_config();
							}
							if((strstr(clibuf+14, "en")))
							{
								config.SHT3_h_use = 1;
								SHT3.hum.use_meas = 1;
								printf("SHTC3 humidity measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+14, "dis")))
							{
								config.SHT3_h_use = 0;
								SHT3.hum.use_meas = 0;
								printf("SHTC3 humidity measure disable\r\n");
								Save_config();
							}
						return;
						}
					}
				}
			}
			if((p = find("ms8607 ")))
			{
				if(p == clibuf+11)
				{
					if((p = find("enable")))
					{
						config.MS8607_use = 1;
						MS8607.sensor_use = 1;
						printf("MS8607 sensor enabled\r\n");
						Save_config();
					}
					if((p = find("disable")))
					{
						config.MS8607_use = 0;
						MS8607.sensor_use = 0;
						printf("MS8607 sensor disabled\r\n");
						Save_config();
					}
					if((p = find("temperature ")))
					{
						if(p == clibuf+23)
						{
							if((strstr(clibuf+23, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+30, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.MS8607_t_offset = tmp;
					            MS8607.temp.offset = tmp;
					            printf("MS8607 temperature offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+23, "en")))
							{
								config.MS8607_t_use = 1;
								MS8607.temp.use_meas = 1;
								printf("MS8607 temperature measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+23, "dis")))
							{
								config.MS8607_t_use = 0;
								MS8607.temp.use_meas = 0;
								printf("MS8607 temperature measure disable\r\n");
								Save_config();
							}
						}
					}
					if((p = find("press ")))
					{
						if(p == clibuf+17)
						{
							if((strstr(clibuf+17, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+24, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.MS8607_p_offset = tmp;
					            MS8607.press.offset = tmp;
					            printf("MS8607 pressure offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+17, "en")))
							{
								config.MS8607_p_use = 1;
								MS8607.press.use_meas = 1;
								printf("MS8607 pressure measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+17, "dis")))
							{
								config.MS8607_p_use = 0;
								MS8607.press.use_meas = 0;
								printf("MS8607 pressure measure disable\r\n");
								Save_config();
							}
						}
					}
					if((p = find("hum ")))
					{
						if(p == clibuf+15)
						{
							if((strstr(clibuf+15, "offset ")))
							{
								float tmp;
						        getFloat(clibuf+22, &tmp, MIN_OFFSET, MAX_OFFSET);
						        config.MS8607_h_offset = tmp;
						        MS8607.hum.offset = tmp;
						        printf("MS8607 humidity offset %.6f\r\n",tmp);
						        Save_config();
							}
							if((strstr(clibuf+15, "en")))
							{
								config.MS8607_h_use = 1;
								MS8607.hum.use_meas = 1;
								printf("MS8607 humidity measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+15, "dis")))
							{
								config.MS8607_h_use = 0;
								MS8607.hum.use_meas = 0;
								printf("MS8607 humidity measure disable\r\n");
								Save_config();
							}
						return;
						}
					}
				}
				return;
			}
			if((p = find("bme280 ")))
			{
				if(p == clibuf+11)
				{
					if((p = find("enable")))
					{
						config.BME280_use = 1;
						BME280.sensor_use = 1;
						printf("BME280 sensor enabled\r\n");
						Save_config();
					}
					if((p = find("disable")))
					{
						config.BME280_use = 0;
						BME280.sensor_use = 0;
						printf("BME280 sensor disabled\r\n");
						Save_config();
					}
					if((p = find("temperature ")))
					{
						if(p == clibuf+23)
						{
							if((strstr(clibuf+23, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+30, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.BME280_t_offset = tmp;
					            BME280.temp.offset = tmp;
					            printf("BME280 temperature offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+23, "en")))
							{
								config.BME280_t_use = 1;
								BME280.temp.use_meas = 1;
								printf("BME280 temperature measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+23, "dis")))
							{
								config.BME280_t_use = 0;
								BME280.temp.use_meas = 0;
								printf("BME280 temperature measure disable\r\n");
								Save_config();
							}
						}
					}
					if((p = find("press ")))
					{
						if(p == clibuf+17)
						{
							if((strstr(clibuf+17, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+24, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.BME280_p_offset = tmp;
					            BME280.press.offset = tmp;
					            printf("BME280 pressure offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+17, "en")))
							{
								config.BME280_p_use = 1;
								BME280.press.use_meas = 1;
								printf("BME280 pressure measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+17, "dis")))
							{
								config.BME280_p_use = 0;
								BME280.press.use_meas = 0;
								printf("BME280 pressure measure disable\r\n");
								Save_config();
							}
						}
					}
					if((p = find("hum ")))
					{
						if(p == clibuf+15)
						{
							if((strstr(clibuf+15, "offset ")))
							{
								float tmp;
						        getFloat(clibuf+22, &tmp, MIN_OFFSET, MAX_OFFSET);
						        config.BME280_h_offset = tmp;
						        BME280.hum.offset = tmp;
						        printf("BME280 humidity offset %.6f\r\n",tmp);
						        Save_config();
							}
							if((strstr(clibuf+15, "en")))
							{
								config.BME280_h_use = 1;
								BME280.hum.use_meas = 1;
								printf("BME280 humidity measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+15, "dis")))
							{
								config.BME280_h_use = 0;
								BME280.hum.use_meas = 0;
								printf("BME280 humidity measure disable\r\n");
								Save_config();
							}
						return;
						}
					}
				}
			}
			if((p = find("dps368 ")))
			{
				if(p == clibuf+11)
				{
					if((p = find("enable")))
					{
						config.DPS368_use = 1;
						DPS368.sensor_use = 1;
						printf("DPS368 sensor enabled\r\n");
						Save_config();
					}
					if((p = find("disable")))
					{
						config.DPS368_use = 0;
						DPS368.sensor_use = 0;
						printf("DPS368 sensor disabled\r\n");
						Save_config();
					}
					if((p = find("temperature ")))
					{
						if(p == clibuf+23)
						{
							if((strstr(clibuf+23, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+30, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.DPS368_t_offset = tmp;
					            DPS368.temp.offset = tmp;
					            printf("DPS368 temperature offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+23, "en")))
							{
								config.DPS368_t_use = 1;
								DPS368.temp.use_meas = 1;
								printf("DPS368 temperature measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+23, "dis")))
							{
								config.DPS368_t_use = 0;
								DPS368.temp.use_meas = 0;
								printf("DPS368 temperature measure disable\r\n");
								Save_config();
							}
						}
					}
					if((p = find("press ")))
					{
						if(p == clibuf+17)
						{
							if((strstr(clibuf+17, "offset ")))
							{
								float tmp;
					            getFloat(clibuf+24, &tmp, MIN_OFFSET, MAX_OFFSET);
					            config.DPS368_p_offset = tmp;
					            DPS368.press.offset = tmp;
					            printf("DPS368 pressure offset %.6f\r\n",tmp);
								Save_config();
							}
							if((strstr(clibuf+17, "en")))
							{
								config.DPS368_p_use = 1;
								DPS368.press.use_meas = 1;
								printf("DPS368 pressure measure enabled\r\n");
								Save_config();
							}
							if((strstr(clibuf+17, "dis")))
							{
								config.DPS368_p_use = 0;
								DPS368.press.use_meas = 0;
								printf("DPS368 pressure measure disable\r\n");
								Save_config();
							}
						return;
						}
					}
				}
			}
		}
		p = find("meas ");
		if(p == clibuf+5)
		{
			if((p = find("start ")))
			{
				if(p == clibuf+11)
				{
					if((strstr(clibuf+11, "txt ")))
					{
						int32_t tmp = -1;
						getval(clibuf+15, &tmp, 1, 500);
						meas_count = tmp;
						meas_cont_mode = 0;
						disp_type = 1;
						printf("Start %i measures, TXT output\r\n", meas_count);
						ReinitTimer(tim_interval);
						return;
					}
					if((strstr(clibuf+11, "csv ")))
					{
						int32_t tmp = -1;
						getval(clibuf+15, &tmp, 1, 500);
						meas_count = tmp;
						meas_cont_mode = 0;
						disp_type = 2;
						printf("Start %i measures, CSV output\r\n", meas_count);
						csvcnt = 0;
						printCSVheader();
						ReinitTimer(tim_interval);
						return;
					}
					if(p == clibuf+11)
					{
						if((strstr(clibuf+11, "cont ")))
						{
							if((strstr(clibuf+16, "txt")))
							{
								meas_cont_mode = 1;
								disp_type = 1;
								printf("Start continuous measurement, TXT format\r\n");
								ReinitTimer(tim_interval);
								return;
							}
							if((strstr(clibuf+16, "csv")))
							{
								meas_cont_mode = 1;
								disp_type = 2;
								printf("Start continuous measurement, TXT format\r\n");
								csvcnt = 0;
								printCSVheader();
								ReinitTimer(tim_interval);
								return;
							}

						}
					}
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



void print_status()
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
//	BQ25798_Chrg_CTRL3_read();
//	BQ25798_Chrg_CTRL4_read();
//	BQ25798_Chrg_FAULT1_read();
//	BQ25798_Chrg_FAULT2_read();
//	BQ25798_Chrg_STAT0_read();
//	BQ25798_Chrg_STAT1_read();
//	BQ25798_Chrg_STAT2_read();
//	BQ25798_Chrg_STAT3_read();
//	BQ25798_Chrg_STAT4_read();
	printf("-----------------\r\n");

}

void print_help()
{
	printf("--- THP HW v%1.1f  FW v%1.1f --- \r\n", HW_VER*0.1f,FW_VER*0.1f );
	printf("SET COMMANDS:\r\n");
	printf("set interval X - X=4..3600[s] - measurement interval\r\n");
	printf("set disptype X - 0 - NONE(silent), 1 - TXT, 2 - CSV - measurement format\r\n");
	printf("set [sensor] enable - sensor=[tmp117;bme280;shtc3;ms8607;dps368] - enable sensor\r\n");
	printf("set [sensor] disable - sensor=[tmp117;bme280;shtc3;ms8607;dps368] - disable sensor\r\n");
	printf("set [sensor] [type] en - type=[temperature;press;hum] - enable sensor type\r\n");
	printf("set [sensor] [type] dis - type=[temperature;press;hum] - disable sensor type\r\n");
	printf("set [sensor] [type] offset X.X - set offset [X.X float]\r\n");
	printf("set [sensor] [type] conf Y - set sensor config [Y - 0..15]\r\n");
	printf("-----------------\r\n");

	printf("CONFIG COMMANDS:\r\n");
	printf("printconfig - Print config values\r\n");
	printf("clearconfig - load default config values\r\n");
	printf("loadconfig - load config values\r\n");
	printf("saveconfig - save config values\r\n");
	printf("-----------------\r\n");

	printf("MEAS COMMANDS:\r\n");
	printf("meas start cont [disp] - Start continuos measurement disp=[txt;csv]\r\n");
	printf("meas start [disp] X - Start X measures disp=[txt;csv], X=1..500 \r\n");

	printf("-----------------\r\n");
	printf("? or help - help\r\n");
	printf("-----------------\r\n");

}
