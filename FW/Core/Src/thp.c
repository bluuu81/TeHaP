/*
 * thp.c
 *
 *  Created on: May 9, 2023
 *      Author: bluuu & Mis
 */

#include "main.h"
#include "sim80xDrv.h"
#include "thp.h"
#include "bq25798.h"
#include "cmsis_os.h"

#define RESEND_DELAY	2000		// w 30ms jednostkach, = 60s
#define RESEND_RETRIES	3

volatile uint8_t charger_state;

TMP117_struct_t TMP117;
SHT3_struct_t SHT3;
MS8607_struct_t MS8607;
BME280_struct_t BME280;
DPS368_struct_t DPS368;
BMP280_HandleTypedef bmp280;

GPRS_status_t GPRS_status_frame;
GPRS_localize_t GPRS_GPS_frame;
GPRS_meas_frame_t GPRS_meas_frame;

//Meas_Send_t Temp_frame;
//Meas_Send_t Press_frame;
//Meas_Send_t Hum_frame;


Config_TypeDef config;
volatile int seconds;
uint8_t timesync;
uint16_t meas_count, sendretry = 0;
uint8_t meas_cont_mode, send_enable, sensors_data_ready;

uint16_t tmp117_avr;
volatile uint8_t dps368_ovr_temp;
volatile uint8_t dps368_ovr_press;
volatile uint16_t dps368_ovr_conf;
uint8_t sht3_mode;

volatile uint32_t offTim;
volatile uint32_t gps_start = 0, gps_interval = 30;			// puerwszy odczyt GPS po 30s od zalaczenia
osThreadId measTaskHandle, gpsTaskHandle, GprsSendTaskHandle;
bool dayleap;

int meas_start;
uint8_t gprs_send_status, lastSendStatus, simOn_lock;

static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

enum {
	GPRSsendStatusIdle,
	GPRSsendStatusInprogress,
	GPRSsendStatusOk,
	GPRSsendStatusError
};

void GpsReadTask(void const *argument);
Sim80x_Time_t fixTZ(Sim80x_Time_t tim, int zone);

//void _close(void) {}
//void _lseek(void) {}
void _read(void)  {}
//void _kill(void)  {}
//void _isatty(void) {}
//void _getpid(void) {}
//void _fstat(void) {}

//volatile uint32_t tim_secdiv, tim_meas;


uint32_t getUID()
{
    uint32_t tmp[3];
    tmp[0] = HAL_GetUIDw0();
    tmp[1] = HAL_GetUIDw1();
    tmp[2] = HAL_GetUIDw2();
    uint32_t hash = FNV_BASIS_32;
    uint8_t *p = (uint8_t*)&tmp;
    for(int i=0; i<12; ++i) hash = (hash * FNV_PRIME_32) ^ *p++;
    return hash;
}

void ReinitTimer(uint16_t interval)
{
	meas_start = seconds;
}

void HAL_SYSTICK_Callback(void)
{
}


void check_powerOn()
{
	uint32_t timon = HAL_GetTick();
	  while(Power_SW_READ() == GPIO_PIN_SET)
	  {
//		  printf("2. Check Power BUT\r\n");
	    if(HAL_GetTick() - timon > 1000) {    // 1 sec pushing
	    	timon = HAL_GetTick();
	        POWER_ON();    // pull-up power supply
	    	printf("Power ON\r\n");
	        break;                // break while loop
	    }
	  }
}

void check_powerOff()
{
  static uint8_t keystate;
  if(Power_SW_READ()) { //power button pressed
	 LED2_ON();
	 keystate = 1;
     if(offTim && HAL_GetTick() - offTim > 2000) {    // 2 sec pressed
    	 printf("Power off\r\n");
    	 LED2_OFF();
    	 LED1_OFF();
		 Sim80x_SetPower(0);
    	 POWER_OFF();
    	 osDelay(2000);
    	 HAL_NVIC_SystemReset();
     }
  } else {
	  offTim = HAL_GetTick();   // button released, update offTim
	  if(keystate) LED2_OFF();
	  keystate = 0;
  }
}

void MCUgoSleep()
{
    // prepare wakeup pin
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2);
    // go sleep
    printf("Sleep\r\n");
    POWER_OFF();
    HAL_PWR_EnterSTANDBYMode();
}

uint8_t HALcalculateCRC(uint8_t* data, uint8_t len)
{
    HAL_CRC_Init(&hcrc);
    __HAL_CRC_DR_RESET(&hcrc);
    uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)data, len);
    return (uint8_t)(crc & 0xFF);
}


uint8_t calculate_crc(uint8_t *data, uint32_t length)
{
    uint8_t crc = 0xFF;
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint8_t calculate_crc_for_GPRS_status(GPRS_status_t *status)
{
    return calculate_crc((uint8_t*)status, (sizeof(GPRS_status_t)-1));
}

uint8_t calculate_crc_for_GPRS_GPS(GPRS_localize_t *status)
{
    return calculate_crc((uint8_t*)status, (sizeof(GPRS_localize_t)-1));
}

uint8_t calculate_crc_for_GPRS_MEAS(GPRS_meas_frame_t *status)
{
    return calculate_crc((uint8_t*)status, (sizeof(GPRS_meas_frame_t)-1));
}

void printCSVheader()
{
	printf("\r\n");
	printf("CNT;");
	if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("TEMP_TMP117;");
	if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("TEMP_BME280;");
	if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("TEMP_SHTC3;");
	if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("TEMP_MS8607;");
	if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("TEMP_DPS368;");

	if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("PRESS_BME280;");
	if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("PRESS_MS8607;");
	if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("PRESS_DPS368;");

	if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("HUM_BME280;");
	if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("HUM_SHTC3;");
	if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("HUM_MS8607;");
	printf("\r\n");

}

void display_values (uint8_t format)
{
	switch (format)
	{
	case 1:
		printf("-----------------------\r\n");
		printf("Temperature:\r\n");
		if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("TMP117: %.2f   ", TMP117.temp.value+TMP117.temp.offset);
		if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("BME280: %.2f   ", BME280.temp.value+BME280.temp.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("SHTC3: %.2f   ", SHT3.temp.value+SHT3.temp.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("MS8607: %.2f   ", MS8607.temp.value+MS8607.temp.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("DPS368: %.2f   ", DPS368.temp.value+DPS368.temp.offset);
		printf("\r\n-----------------------\r\n");
		printf("Press:\r\n");
		if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("BME280: %.2f   ", BME280.press.value+BME280.press.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("MS8607: %.2f   ", MS8607.press.value+MS8607.press.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("DPS368: %.2f   ", DPS368.press.value+DPS368.press.offset);
		printf("\r\n-----------------------\r\n");
		printf("Hum:\r\n");
		if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("BME280: %.2f   ", BME280.hum.value+BME280.hum.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("SHTC3: %.2f   ", SHT3.hum.value+SHT3.hum.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("MS8607: %.2f   ", MS8607.hum.value+MS8607.hum.offset);
		printf("\r\n-----------------------\r\n");
		break;

	case 2:
		printf("%u;",++csvcnt);
		if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("%.2f;",TMP117.temp.value+TMP117.temp.offset);
		if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("%.2f;",BME280.temp.value+BME280.temp.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("%.2f;",SHT3.temp.value+SHT3.temp.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("%.2f;",MS8607.temp.value+MS8607.temp.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("%.2f;",DPS368.temp.value+DPS368.temp.offset);

		if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("%.2f;",BME280.press.value+BME280.press.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("%.2f;",MS8607.press.value+MS8607.press.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("%.2f;",DPS368.press.value+DPS368.press.offset);

		if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("%.2f;",BME280.hum.value+BME280.hum.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("%.2f;",SHT3.hum.value+SHT3.hum.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("%.2f;",MS8607.hum.value+MS8607.hum.offset);
		printf("\r\n");
		break;

	default:
		break;
	}

}

void getConfVars()
{
	  TMP117.sensor_use = config.TMP117_use;
	  SHT3.sensor_use = config.SHT3_use;
	  MS8607.sensor_use = config.MS8607_use;
	  BME280.sensor_use = config.BME280_use;
	  DPS368.sensor_use = config.DPS368_use;

	  TMP117.sensor_conf = config.TMP117_conf;
	  TMP117.temp.use_meas = config.TMP117_t_use;
	  TMP117.temp.offset = config.TMP117_t_offset;

	  BME280.sensor_conf = config.BME280_conf;
	  BME280.temp.use_meas = config.BME280_t_use;
	  BME280.temp.offset = config.BME280_t_offset;

	  SHT3.sensor_conf = config.SHT3_conf;
	  SHT3.temp.use_meas = config.SHT3_t_use;
	  SHT3.temp.offset = config.SHT3_t_offset;

	  MS8607.sensor_conf = config.MS8607_conf;
	  MS8607.temp.use_meas = config.MS8607_t_use;
	  MS8607.temp.offset = config.MS8607_t_offset;

	  DPS368.sensor_conf = config.DPS368_conf;
	  DPS368.temp.use_meas = config.DPS368_t_use;
	  DPS368.temp.offset = config.DPS368_t_offset;

	  BME280.press.use_meas = config.BME280_p_use;
	  BME280.press.offset = config.BME280_p_offset;
	  MS8607.press.use_meas = config.MS8607_p_use;
	  MS8607.press.offset = config.MS8607_p_offset;
	  DPS368.press.use_meas = config.DPS368_p_use;
	  DPS368.press.offset = config.DPS368_p_offset;

	  BME280.hum.use_meas = config.BME280_h_use;
	  BME280.hum.offset = config.BME280_h_offset;
	  SHT3.hum.use_meas = config.SHT3_h_use;
	  SHT3.hum.offset = config.SHT3_h_offset;
	  MS8607.hum.use_meas = config.MS8607_h_use;
	  MS8607.hum.offset = config.MS8607_h_offset;
}

uint32_t time_to_unix(Sim80x_Time_t *tm)
{
	int i;
	uint32_t seconds;
	uint16_t year = tm->Year-1970;

	// seconds from 1970 till 1 jan 00:00:00 of the given year
	seconds= year*(SECS_PER_DAY * 365);
	for (i = 0; i < year; i++) {
		if (LEAP_YEAR(i)) {
		  seconds +=  SECS_PER_DAY;   // add extra days for leap years
		}
	}

	// add days for this year, months start from 1
	for (i = 1; i < tm->Month; i++) {
		if ( (i == 2) && LEAP_YEAR(year)) {
		  seconds += SECS_PER_DAY * 29;
		} else {
		  seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
		}
	}
	seconds+= (tm->Day-1) * SECS_PER_DAY;
	seconds+= tm->Hour * SECS_PER_HOUR;
	seconds+= tm->Min * SECS_PER_MIN;
	seconds+= tm->Sec;
	return (uint32_t)seconds;
}

// ******************************************************************************************************

bool SendPkt(uint8_t *ptr, uint16_t len, char *text)
{
	Sim80x.GPRS.SendStatus = GPRSSendData_Idle;
	for(int i=0; i<5; ++i) {
		GPRS_SendRaw(ptr,len);
		uint8_t tout = 0;
		while(Sim80x.GPRS.SendStatus != GPRSSendData_SendOK) {
			osDelay(100);
			if(++tout >= 100) break;			// break while - tout 10 sekund
		}
		if(tout < 100) {printf("Sending %s OK !\r\n", text); break;}	// break for
	}

	if(Sim80x.GPRS.SendStatus != GPRSSendData_SendOK) {
		printf("GPRS Sending %s Failed !\r\n", text);
		gprs_send_status = GPRSsendStatusError;			// ustaw globalny status wysylania na error
		return false;
	} else if(gprs_send_status < GPRSsendStatusOk) gprs_send_status = GPRSsendStatusOk;
	osDelay(1000);
	return true;
}

uint32_t GetCurrTimestamp()
{
#define USE_UTC

	Sim80x_GetTime();		// pobranie czasu z RTC sim868
#ifdef USE_UTC
	Sim80x_Time_t Ti = fixTZ(Sim80x.Gsm.Time, -Sim80x.Gsm.Time.Zone);
#else
	Sim80x_Time_t Ti = Sim80x.Gsm.Time;
#endif
	return time_to_unix(&Ti);
}


void PrepareGpsPacket()
{
	// wypelnienie struktury GPS
	GPRS_GPS_frame.timestamp = GetCurrTimestamp(); printf("Timestamp (GPS): %lu\r\n", GPRS_GPS_frame.timestamp);
	GPRS_GPS_frame.lat = (float)(Sim80x.GPS.Lat * 0.01f); printf("Latitude (GPS): %.6f\r\n", GPRS_GPS_frame.lat);
	GPRS_GPS_frame.lon = (float)(Sim80x.GPS.Lon * 0.01f); printf("Longtitude (GPS): %.6f\r\n", GPRS_GPS_frame.lon);
	GPRS_GPS_frame.sats = Sim80x.GPS.SatInUse; printf("SAT count: %u\r\n", GPRS_GPS_frame.sats);
	GPRS_GPS_frame.fix = Sim80x.GPS.Fix; printf("FIX type: %u\r\n", GPRS_GPS_frame.fix);
	GPRS_GPS_frame.send_type = LOCALIZE; printf("Frame type (GPS): %u\r\n", GPRS_GPS_frame.send_type);
	GPRS_GPS_frame.crc = calculate_crc_for_GPRS_GPS(&GPRS_GPS_frame); printf("CRC (GPS): %x\r\n", GPRS_GPS_frame.crc);
}

void PreparePacketOutData()
{
#define USE_UTC

	printf("UID: %lx\r\n", GPRS_status_frame.UID);
	printf("Token: %x\r\n", GPRS_status_frame.token);

	GPRS_status_frame.timestamp = GetCurrTimestamp(); printf("Timestamp (status): %lu\r\n", GPRS_status_frame.timestamp);
	GPRS_status_frame.MCU_temp = GET_MCU_Temp(); printf("MCU Temp: %.2f\r\n", GPRS_status_frame.MCU_temp);
	GPRS_status_frame.send_type = STATUS; printf("Frame type (status): %u\r\n", GPRS_status_frame.send_type);
	GPRS_status_frame.Vac1 = BQ25798_Vac2_read(); printf("AC1 Voltage: %u\r\n", GPRS_status_frame.Vac1);
	GPRS_status_frame.Vac2 = BQ25798_Vac2_read(); printf("AC2 Voltage: %u\r\n", GPRS_status_frame.Vac2);
	GPRS_status_frame.Vbat = BQ25798_Vbat_read(); printf("BAT Voltage: %u\r\n", GPRS_status_frame.Vbat);
	GPRS_status_frame.charg_state = charger_state; printf("Chrg state: %u\r\n", GPRS_status_frame.charg_state);
	GPRS_status_frame.crc = calculate_crc_for_GPRS_status(&GPRS_status_frame); printf("CRC (status): %x\r\n", GPRS_status_frame.crc);

	printf("Wielkosc ramki (status): %u\r\n", sizeof(GPRS_status_frame));
	printf("---------------------------\r\n");

	GPRS_GPS_frame.send_type = LOCALIZE;
	GPRS_GPS_frame.crc = calculate_crc_for_GPRS_GPS(&GPRS_GPS_frame);


}

void SendTestMessage()
{
	// Sending status
	if(!SendPkt((uint8_t*)&GPRS_status_frame, sizeof(GPRS_status_frame), "status")) return;

	// Sending localize
	if(!SendPkt((uint8_t*)&GPRS_GPS_frame, sizeof(GPRS_GPS_frame), "localize")) return;

//	GPRS_meas_frame.timestamp = GPRS_status_frame.timestamp;    // to wystarczy tylko raz podstawić
	GPRS_meas_frame.timestamp = GetCurrTimestamp(); printf("Timestamp (Meas): %lu\r\n", GPRS_meas_frame.timestamp);

	// Fill temp values to send frame
	if(TMP117.present && TMP117.sensor_use){
	  if(TMP117.temp.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor1_val = TMP117.temp.value+TMP117.temp.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor1_val = 0.0f;

	if(BME280.present && BME280.sensor_use){
	  if(BME280.temp.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor2_val = BME280.temp.value+BME280.temp.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor2_val = 0.0f;

	if(DPS368.present && DPS368.sensor_use){
	  if(DPS368.temp.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor3_val = DPS368.temp.value+DPS368.temp.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor3_val = 0.0f;


	GPRS_meas_frame.meas_frame.send_type = TEMP;
	GPRS_meas_frame.crc = calculate_crc_for_GPRS_MEAS(&GPRS_meas_frame); printf("CRC (TEMP): %x\r\n", GPRS_meas_frame.crc);

	// Sending Temperature
	if(!SendPkt((uint8_t*)&GPRS_meas_frame, sizeof(GPRS_meas_frame), "TEMP")) return;
	printf("Wielkosc ramki (TEMP): %u\r\n", sizeof(GPRS_meas_frame));
	printf("---------------------------\r\n");



	// Fill temp values to send frame
	if(BME280.present && BME280.sensor_use){
	  if(BME280.press.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor1_val = BME280.press.value+BME280.press.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor1_val = 0.0f;

	if(MS8607.present && MS8607.sensor_use){
	  if(MS8607.press.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor2_val = MS8607.press.value+MS8607.press.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor2_val = 0.0f;

	if(DPS368.present && DPS368.sensor_use){
	  if(DPS368.press.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor3_val = DPS368.press.value+DPS368.press.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor3_val = 0.0f;

	GPRS_meas_frame.meas_frame.send_type = PRESS;
	GPRS_meas_frame.crc = calculate_crc_for_GPRS_MEAS(&GPRS_meas_frame); printf("CRC (PRESS): %x\r\n", GPRS_meas_frame.crc);

	// Sending Pressure
	if(!SendPkt((uint8_t*)&GPRS_meas_frame, sizeof(GPRS_meas_frame), "PRESS")) return;
	printf("Wielkosc ramki (PRESS): %u\r\n", sizeof(GPRS_meas_frame));
	printf("---------------------------\r\n");

	// Fill hum values to send frame
	if(BME280.present && BME280.sensor_use){
	  if(BME280.hum.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor1_val = BME280.hum.value+BME280.hum.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor1_val = 0.0f;

	if(SHT3.present && SHT3.sensor_use){
	  if(SHT3.hum.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor2_val = SHT3.hum.value+SHT3.hum.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor2_val = 0.0f;

	if(MS8607.present && MS8607.sensor_use){
	  if(MS8607.hum.use_meas) {
		 GPRS_meas_frame.meas_frame.sensor3_val = MS8607.hum.value+MS8607.hum.offset;
	  }
	} else GPRS_meas_frame.meas_frame.sensor3_val = 0.0f;

	GPRS_meas_frame.meas_frame.send_type = HUM;
	GPRS_meas_frame.crc = calculate_crc_for_GPRS_MEAS(&GPRS_meas_frame); printf("CRC (HUM): %x\r\n", GPRS_meas_frame.crc);

	// Sending Humidity
	if(!SendPkt((uint8_t*)&GPRS_meas_frame, sizeof(GPRS_meas_frame), "HUM")) return;
	printf("Wielkosc ramki (HUM): %u\r\n", sizeof(GPRS_meas_frame));
	printf("---------------------------\r\n");



	osDelay(5000);			// normalnie zbedny, ale to dla mozliwosci odebrania danych z serwera.
							// zamiast tego powinno byc oczekiwanie na potwierdzenie z serwera (jesli wystepuje)
}

void SendMqttMessage(void)
{
	char tekst[100];
	Sim80x_GetTime();		// pobranie czasu z RTC sim868
	sprintf(tekst, "Test Wysylania do MQTT, Czas: %02u:%02u:%02u\r\n",
			Sim80x.Gsm.Time.Hour, Sim80x.Gsm.Time.Min, Sim80x.Gsm.Time.Sec);

	SendPkt((uint8_t*)tekst, strlen(tekst), "MQTT");
	osDelay(5000);			// normalnie zbedny, ale to dla mozliwosci odebrania danych z serwera.
							// zamiast tego powinno byc oczekiwanie na potwierdzenie z serwera (jesli wystepuje)
}

void GprsSendTask(void const *argument)
{
	printf("GPRS task started.\r\n");
	while(1) {
		vTaskSuspend(NULL);		// zatrzymaj taki i czekaj na komende start
		// ustaw globalny status wysylania na "in progress"
		gprs_send_status = GPRSsendStatusInprogress;
		lastSendStatus = gprs_send_status;
		osDelay(300);
		while(Sim80x.Status.LockSlowRun) osDelay(10);
		LockSlowRun();
		if(Sim80x.GPRS.Connection < GPRSConnection_GPRSup ||
		   Sim80x.GPRS.Connection > GPRSConnection_ConnectOK)	{		// nie polaczony do GPRS
			bool status = GPRS_ConnectToNetwork("INTERNET", "", "", false);
			printf("Connect to network: %s\r\n", status ? "OK":"ERROR");
			printf("Connected to GPRS, IP: %s\r\n", Sim80x.GPRS.LocalIP);
			osDelay(250);
			if(!status) {printf("GPRS ERROR\r\n"); gprs_send_status = GPRSsendStatusError; goto error;}
		} else printf("GPRS is UP\r\n");

		if(config.sendFormat & 1) {				// normal send
			bool status = GPRS_ConnectToServer(config.serverIP, config.serverPort);
			printf("Connecting to server: %s, port %d - %s\r\n", config.serverIP, config.serverPort, status ? "IN PROGRESS":"ERROR");
			if(!status) {gprs_send_status = GPRSsendStatusError; goto error;}
			uint8_t tout = 0;
			while(Sim80x.GPRS.Connection != GPRSConnection_ConnectOK)	{	// gotowy do wysylania danych ?
				osDelay(100);
				if(++tout >= 100) break;			// timeout na 10 sekund
			}
			if(tout < 100) {						// nie bylo timeout
				printf("Connected !\r\n");
				SendTestMessage();					// serwer połączony, pogadaj z nim
				GPRS_DisconnectFromServer();		// rozlacz od serwera
			} else {gprs_send_status = GPRSsendStatusError; printf("ERROR, Server not respond\r\n");}
		}
		if(config.sendFormat & 2) {				// MQTT send
			bool status = GPRS_ConnectToServer(config.mqttIP, config.mqttPort);
			printf("Connecting to MQTT: %s, port %d - %s\r\n", config.mqttIP, config.mqttPort, status ? "IN PROGRESS":"ERROR");
			if(!status) {gprs_send_status = GPRSsendStatusError; goto error;}
			uint8_t tout = 0;
			while(Sim80x.GPRS.Connection != GPRSConnection_ConnectOK)	{	// gotowy do wysylania danych ?
				osDelay(100);
				if(++tout >= 100) break;			// timeout na 10 sekund
			}
			if(tout < 100) {						// nie bylo timeout
				printf("Connected !\r\n");
				SendMqttMessage();					// serwer MQTT połączony, pogadaj z nim
				GPRS_DisconnectFromServer();		// rozlacz od serwera
			} else {gprs_send_status = GPRSsendStatusError; printf("ERROR, MQTT Server not respond\r\n");}
		}
error:
		osDelay(300);
		GPRS_DeactivatePDPContext();				// wylacz GPRS
		osDelay(50);
		UnlockSlowRun();
		lastSendStatus = gprs_send_status;
		gprs_send_status = GPRSsendStatusIdle;
	}
}

// ******************************************************************************************************

void ResetGprsTask()
{
	if(GprsSendTaskHandle != NULL) {
		vTaskSuspend(GprsSendTaskHandle);
		osDelay(5);
		vTaskDelete(GprsSendTaskHandle);
		osDelay(5);
		printf("GPRS task killed.\r\n");
		osThreadDef(SendGPRSTask, GprsSendTask, osPriorityNormal, 0, 512);
		GprsSendTaskHandle = osThreadCreate(osThread(SendGPRSTask), NULL);
		osDelay(5);
	}
}

void ResetGpsTask()
{
	if(gpsTaskHandle != NULL) {
		vTaskSuspend(gpsTaskHandle);
		osDelay(5);
		vTaskDelete(gpsTaskHandle);
		osDelay(5);
		printf("GPS task killed.\r\n");
		osThreadDef(GPSTask, GpsReadTask, osPriorityNormal, 0, 256);
		gpsTaskHandle = osThreadCreate(osThread(GPSTask), NULL);
		osDelay(5);
	}
}

bool StartSendGPRS(void)
{
	static uint32_t lastStartTime;

    lastSendStatus = GPRSsendStatusIdle;
	bool running = (eTaskGetState(GprsSendTaskHandle) == eRunning);
	if(running && HAL_GetTick()-lastStartTime >= 3*60000) {			// jak poprzednie uruchomienie bylo 3min temu i wiecej
		ResetGprsTask();											// i task dalej dziala, to go zabij
		lastStartTime = HAL_GetTick();
		running = false;
	}
	if(!running && Sim80x.Status.RegisterdToNetwork && config.sendFormat) {
		if((config.sendFormat & 1) && (config.serverIP[0]==0 || config.serverPort==0)) {
			printf("Normal server param error, IP: %s, Port: %d\r\n", config.serverIP, config.serverPort);
			return false; // blad IP/Port normal
		}
		if((config.sendFormat & 2) && (config.mqttIP[0]==0 || config.mqttPort==0)) {
			printf("MQTT server param error, IP: %s, Port: %d\r\n", config.serverIP, config.serverPort);
			return false; // blad IP/Port MQTT
		}
		vTaskResume(GprsSendTaskHandle);
		lastStartTime = HAL_GetTick();
		return true; // poprawnie uruchomiono task
	}
	printf("Start debug: Task running: %d, RegisterdToNetwork: %d, sendFormat: %d\r\n", running, Sim80x.Status.RegisterdToNetwork, config.sendFormat);
	return false; // nie uruchomiono tasku bo juz działa
}

void  GPRS_UserNewData(char *NewData, uint16_t len)		// callback dla danych przyjetych z serwera
{
	_write(0, NewData, len);
	printf("\r\n");
}

// *******************************************************************************************
// SIM868 watchdog & autorestart

void GsmWdt(void)
{
	static uint8_t  gsm_led_state;
	static uint16_t gsm_wdt;
#if(GSM_RESTART_INTERVAL > 0)
	static uint32_t gsm_restart_time = GSM_RESTART_INTERVAL;
#endif
	// watchdog dla GSM
	gsm_wdt++;
	if(!gsm_led_state && SIM_WDT_READ()) {
		gsm_led_state = 1;
		gsm_wdt = 0;
	}
	if(gsm_led_state && !SIM_WDT_READ()) gsm_led_state = 0;

	if(Sim80x.Status.Power && gsm_wdt > 300) {		// 10 sekund WDT timeout
		printf("GSM module recovery!\r\n");
		Sim80x.Status.FatalError = 1;
	} else if(!Sim80x.Status.Power) gsm_wdt = 0;

#if(GSM_RESTART_INTERVAL > 0)
    if(gsm_restart_time) gsm_restart_time--;
    if(gsm_restart_time == 0) {						// cykliczny restart SIM868 co 24h
        uint8_t msg_in_process = 0;
        for(uint8_t i=0 ;i<sizeof(Sim80x.Gsm.HaveNewMsg); ++i) {
            if(Sim80x.Gsm.HaveNewMsg[i] > 0) msg_in_process = 1;
        }

        if(!Sim80x.GPS.RunStatus &&                                 // GPS OFF
           Sim80x.GPRS.Connection == GPRSConnection_Idle &&         // Nie ma komunikacji GPRS
           !Sim80x.Status.Busy &&                                   // Nie jest obslugiwana komenda AT
           Sim80x.Gsm.GsmVoiceStatus == GsmVoiceStatus_Idle &&      // Nie trwa polaczenie voice
           Sim80x.Gsm.MsgUsed == 0 &&                               // nie ma w pamieci nieobsluzonych SMS
           msg_in_process == 0) {                                   // nie jest obslugiwany przychodzacy SMS
            gsm_restart_time = GSM_RESTART_INTERVAL;
            Sim80x.Status.FatalError = 1;                           // wymus pelny restart SIM800
            printf("Sheduled GSM restart.\r\n");
        } else gsm_restart_time = 10*5;                             // nie wolno restartowac, kolejny test za 5s.
    }
#endif

	if(Sim80x.Status.FatalError) {			// odpal restart GSM
		LockSlowRun();
		printf("Restarting...\r\n");
		HAL_GPIO_WritePin(_SIM80X_POWER_KEY_GPIO,_SIM80X_POWER_KEY_PIN,GPIO_PIN_SET);
		osDelay(1200);
		printf("RST 2\r\n");
		HAL_GPIO_WritePin(_SIM80X_POWER_KEY_GPIO,_SIM80X_POWER_KEY_PIN,GPIO_PIN_RESET);
		osDelay(4000);
		printf("GSM power UP.\r\n");
		memset(&Sim80x,0,sizeof(Sim80x));
		Sim80x_SetPower(true);
		osDelay(100);
		ResetGprsTask();
		ResetGpsTask();
		gps_start = 0;
		gps_interval = 30;					// odczyt GPS po 30s od restartu
		meas_start = -1;
		timesync = 0;
        gsm_wdt = 0;
        lastSendStatus = GPRSsendStatusIdle;
        UnlockSlowRun();
	}
}

// ******************************************************************************************************

Sim80x_Time_t fixTZ(Sim80x_Time_t tim, int zone)		// adjust time with time zone. Use GSM format, zone = 15min
{
	#define LEAP_YEAR_SIM(Y)  ( !(Y%4) && ( (Y%100) || !(Y%400) ) )
	static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
	uint8_t monthLength;
	for(int i=0; i<abs(zone); ++i) {
		int Min = (int)tim.Min + ((zone < 0) ? -15:15);
		if(zone > 0 && Min > 59) {
			Min -= 60;
			if(++tim.Hour > 23)	{
				tim.Hour = 0;
				if (tim.Month==2) { 		// luty
				  if (LEAP_YEAR_SIM(tim.Year)) { monthLength=29; } else { monthLength=28; }		// luty ma 28 czy 29 ?
				} else { monthLength = monthDays[tim.Month-1]; }							// inny miesiac
				if(++tim.Day > monthLength) {												// zmiana miesiaca ?
					tim.Day = 1;
					if(++tim.Month > 12) {tim.Month = 1; tim.Year++;}
				}
			}
		} else if(zone < 0 && Min < 0) {
			Min += 60;
			int Hour = tim.Hour;			// konwersja na int ze znakiem aby wykryć ujemne wartosci godzin
			if(--Hour < 0) {
				Hour = 23;
				if(--tim.Day < 1) {
					if(--tim.Month < 1)  {tim.Month = 12; tim.Year--;}							// oblucz nowy miesiac i rok
					if (tim.Month==2) { 														// jak wyszedl luty
						if (LEAP_YEAR_SIM(tim.Year)) { monthLength=29; } else { monthLength=28; }	// luty ma 28 czy 29 ?
					} else { monthLength = monthDays[tim.Month-1]; }							// inny miesiac
					tim.Day = monthLength;								// ustaw na ostatni dzien miesiaca
				}
			}
			tim.Hour = Hour;
		}
		tim.Min = Min;
	}
	tim.Zone += zone;
	return tim;
}

void GpsReadTask(void const *argument)
{
	printf("GPS task started.\r\n");
	while(1) {
		vTaskSuspend(NULL);		// zatrzymaj taki i czekaj na komende start
		while(gprs_send_status != GPRSsendStatusIdle) osDelay(1000);
		uint8_t time_updated = 0;
		printf("GPS start.\r\n");
		GPS_SetPower(1);
		uint16_t GPS_tout = 0;
		while(1) {
			osDelay(100);
			if(gprs_send_status != GPRSsendStatusIdle) {printf("Stop GPS task due to GPRS comm.\r\n"); goto gpstaskend;}
			if(Sim80x.GPS.Time.Year > 2022 && !time_updated && Sim80x.GPS.Fix) {		// prawidlowy czas
			   																			// i jeszcze nie uaktualniony
			  																			// i jest fix
				if(Sim80x.Gsm.Time.Zone == 0) Sim80x.Gsm.Time.Zone = 8;					// tu male oszustwo ze strefą czasową
				Sim80x.Gsm.Time = fixTZ(Sim80x.GPS.Time, Sim80x.Gsm.Time.Zone);
				time_updated = Sim80x_SetTime();
				printf("GSM Time update %s.\r\n", time_updated ? "OK":"ERROR");
				printf("Current time is: %04u-%02u-%02u %02u:%02u:%02u, TZ:%d\r\n",
						Sim80x.Gsm.Time.Year, Sim80x.Gsm.Time.Month, Sim80x.Gsm.Time.Day,
						Sim80x.Gsm.Time.Hour, Sim80x.Gsm.Time.Min, Sim80x.Gsm.Time.Sec, Sim80x.Gsm.Time.Zone);
			}
			if(time_updated && Sim80x.GPS.Fix && Sim80x.GPS.SatInUse > 3) break;
			if(++GPS_tout > 1800 || Sim80x.GPS.RunStatus == 0) {		// 3 minuty timeout
				printf("GPS signal not available.");
				goto gpstaskend;
			}
		}
		while(gprs_send_status != GPRSsendStatusIdle) osDelay(1000);	// nie wysylaj nic do GPS jak polaczony z GPRS
		Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK285,1,10*0D\"\r\n", 500, 1,"\r\nOK\r\n");		// 10ms BLUE blink
		printf("FIX ok, position readed.\r\n");
		osDelay(15000);									// jeszcze przez 15 sekund czytaj GPS
		PrepareGpsPacket();								// wygeneruj pakiet GPRS z danymi GPS

gpstaskend:
		GPS_SetPower(0);
		printf("GPS stopped.\r\n");
	}
}

bool StartReadGps(void)
{
	if(eTaskGetState(gpsTaskHandle) != eRunning && gprs_send_status == GPRSsendStatusIdle) {
		vTaskResume(gpsTaskHandle);
		return true;			// poprawnie uruchomiono task
	}
	return false;				// nie uruchomiono tasku bo juz działa, albo jest polaczony z GPRS
}

// ******************************************************************************************************

void SensorsTask(void const *argument)
{
	uint8_t shtc3_values[6];

	printf("Sensors task created\r\n");
	while(1)
	{
		vTaskSuspend(NULL);		// zatrzymaj taki i czekaj na komende start
		LED2_ON();				// mrugniecie czerwona
		uint32_t meas_time = 0;
		uint8_t dps368_press = 0;

		// uruchomienie pomiaru w poszczegolnych czujnikach
		if(TMP117.present){
		  if(TMP117.sensor_use && TMP117.temp.use_meas) {
			  TMP117_start_meas(tmp117_avr);
			  if(meas_time < 200) meas_time = 200;
		  }
		}
		if(BME280.present){
		  if(BME280.sensor_use && (BME280.temp.use_meas || BME280.press.use_meas || BME280.hum.use_meas) ) {
			  BME280_start_meas();
			  if(meas_time < 500) meas_time = 500;
		  }
		}
		if(SHT3.present){
		  if(SHT3.sensor_use && (SHT3.temp.use_meas || SHT3.hum.use_meas)) {
			  SHTC3_start_meas(sht3_mode);
			  if(meas_time < 100) meas_time = 100;
		  }
		}
		if(DPS368.present){
		  if(DPS368.sensor_use && (DPS368.temp.use_meas || DPS368.press.use_meas)) {
			  DPS368_start_meas_temp(dps368_ovr_temp);
			  uint32_t dpstim = calcBusyTime(dps368_ovr_temp);
			  if(meas_time < dpstim) meas_time = dpstim;
			  if(DPS368.press.use_meas) dps368_press = 1;				// z DPS bedzie tez cisnienie
		  }
		}
		if(config.disp_type == 1) {
		  printf("Komenda startu pomiarow wyslana\r\n");
		  if(!meas_cont_mode) printf("Meas count: %u\r\n", meas_count);
		}

		osDelay(10);
		LED2_OFF();						// mrugniecie czerwona
		osDelay(meas_time);				// odczekaj czas potrzebny na przetworzenie (maksymalny wymagany)

		// odczyt danych z czujników
		if(TMP117.present && TMP117.sensor_use){
		  if(TMP117.temp.use_meas) {
			  TMP117.temp.value = TMP117_get_temp();
		//    			  printf("Temperatura TMP117: %.2f\r\n", TMP117.temp.value);
		  }
		}
		if(BME280.present && BME280.sensor_use){
		  if(BME280.temp.use_meas) {
			  BME280.temp.value = BME280_get_temp();
		//    			  printf("Temperatura BME280: %.2f\r\n", BME280.temp.value);
		  }
		  if(BME280.press.use_meas) {
			  BME280.press.value = BME280_get_press();
		//    			  printf("Cisnienie BME280: %.2f\r\n", BME280.press.value);
		  }
		  if(BME280.hum.use_meas) {
			  BME280.hum.value = BME280_get_hum();
		//    		      printf("Wilgotnosc BME280: %.2f\r\n", BME280.hum.value);
		  }
		}
		if(SHT3.present && SHT3.sensor_use){
		  SHTC3_read_values(shtc3_values);
		  if(SHT3.temp.use_meas) {
			  SHT3.temp.value = SHTC3_get_temp(shtc3_values);
		//    			  printf("Temperatura SHT3: %.2f\r\n", SHT3.temp.value);
		  }
		  if(SHT3.hum.use_meas) {
			  SHT3.hum.value = SHTC3_get_hum(shtc3_values);
		//    			  printf("Wilgotnosc SHT3: %.2f\r\n", SHT3.hum.value);
		  }
		}
		if(MS8607.present && MS8607.sensor_use){
		  if(MS8607.temp.use_meas) {
			  MS8607.temp.value = MS8607_get_temp();
		//    			  printf("Temperatura MS8607: %.2f\r\n", MS8607.temp.value);
		  }
		  if(MS8607.press.use_meas) {
			  MS8607.press.value = MS8607_get_press();
		//    			  printf("Cisnienie MS8607: %.2f\r\n", MS8607.press.value);
		  }
		  if(MS8607.hum.use_meas) {
			  MS8607.hum.value = MS8607_get_hum();
		//    			  printf("Wilgotnosc MS8607: %.2f\r\n", MS8607.hum.value);
		  }
		}
		if(DPS368.present && DPS368.sensor_use){
		  float dps_scaled_temp = DPS368_get_scaled_temp();				// odczytaj temperature
		  if(DPS368.temp.use_meas) {
			  DPS368.temp.value = DPS368_calc_temp(dps_scaled_temp);
		//    			  printf("Temperatura DPS368: %.2f\r\n", DPS368.temp.value);
		  }
		  if(dps368_press) {											// jak ma byc cisnienie z DPS
			  DPS368_start_meas_press(dps368_ovr_press);				// uruchom przetworzenie cisnienia
			  osDelay( calcBusyTime(dps368_ovr_press) + 10);			// zaczekaj na koniec przetwarzania
   			  DPS368.press.value = DPS368_get_press(dps_scaled_temp);	// pobierz cisnienie uzywając temperatury
//    		  printf("Cisnienie DPS368: %.2f\r\n", DPS368.press.value);
		  }
		}
		sensors_data_ready = 1;
	}		// while(1)
}

// ******************************************************************************************************

void SysTimeSync()
{
    // synchronizacja soft rtc
    Sim80x_GetTime();
    if(Sim80x.Gsm.Time.Year > 2022) {
		int gsmsec = Sim80x.Gsm.Time.Hour * 3600 + Sim80x.Gsm.Time.Min * 60 + Sim80x.Gsm.Time.Sec;
		if(seconds != gsmsec) {
		  seconds = gsmsec;
		  timesync = 1;
		}
    }
}

void CalculateNextMeasTime()
{
	printf("Current sys time: %02d:%02d:%02d\r\n", seconds/3600, (seconds%3600)/60, seconds%60);
	printf("Send interval: %d min, Meas count: %d, Meas Interval: 5s\r\n", config.tim_interval, config.measures);
	int nextsend = ((seconds + 60*config.tim_interval + 60)/900) * 900;	// aktualny + interval zaokraglony do 15min
	printf("Next send at: %02d:%02d:%02d\r\n", nextsend/3600, (nextsend%3600)/60, nextsend%60);
	nextsend -= config.measures * 5;						// odejmij czas potrzebny na pomiary
	if(nextsend < 0 ) nextsend += SEC_PER_DAY;				// jak < 0 to start przed polnoca
	if(nextsend <= seconds) {								// za malo czasu od teraz do startu
		nextsend += 60*config.tim_interval;					// dodaj jeden interwal
		printf("Advance by one send interval\r\n");
	}
	if(nextsend >= SEC_PER_DAY) {
		nextsend -= SEC_PER_DAY;							// wysylka bedzie w kolejnym dniu
		dayleap = true;										// blokada do czasu zmiany dnia
	}
	meas_start = nextsend;
	printf("Next measure start at: %02d:%02d:%02d\r\n", nextsend/3600, (nextsend%3600)/60, nextsend%60);
}

void THP_MainTask(void const *argument)
{
	  POWER_OFF();
	  if(!Power_SW_READ()) HAL_NVIC_SystemReset();		// nie nacisniety power -> reset CPU
	  HAL_UART_RxCpltCallback(&huart1); //CLI
	  HAL_UART_RxCpltCallback(&huart2); //SIM
	  check_powerOn();
	  if(!Power_SW_READ()) HAL_NVIC_SystemReset();		// nie nacisniety power -> reset CPU
	  printf("\r\n\r\n\r\nInitializing (RTOS version)...\r\n");
	  if (Load_config()==0) {printf("Config loaded OK \r\n");};
	  charger_state = BQ25798_check();
	  if (charger_state) {
		  printf("Configure charger \r\n");
		  QON_EN();
		  BQ25798_Sys_Min_Voltage_write(3); 	// 3250mV
		  BQ25798_Chr_Volt_Limit_write(4200); 	// 4200mV
		  BQ25798_Chr_Curr_Limit_write(2000); 	// 2000mA
		  BQ25798_Chr_Input_Voltage_Limit_write(130); //*100mV
		  BQ25798_Chr_Input_Curr_Limit_write(200); //*10mA
		  BQ25798_Chrg_CTRL1_write(0x95);
		  BQ25798_Chrg_NTC_CTRL1_write(1);
		  CE_EN();
		  BQ25798_MPPT_CTRL(1); //MPPT ON

	  }
	  LED1_ON();
	  LED2_OFF();
	  ADC_DMA_Start();

	  TMP117.present = TMP117_check();
	  SHT3.present = SHTC3_check();
	  MS8607.present = MS8607_check();
	  BME280.present = BME280_check();
	  DPS368.present = DPS368_check();

	  getConfVars();

	  tmp117_avr=tmp117_avr_conf(TMP117.sensor_conf);
	//  printf("TMP117 conf var %x\r\n", tmp117_avr);
	  dps368_ovr_conf=dps368_ovr_config(DPS368.sensor_conf);
	  printf("DPS368 conf var %x\r\n", dps368_ovr_conf);
	  dps368_ovr_temp = (uint8_t)(dps368_ovr_conf >> 8);
	  dps368_ovr_press = (uint8_t)dps368_ovr_conf;

	  DPS368_init(FIFO_DIS, INT_NONE);
	  DPS368_temp_correct(dps368_ovr_temp);

	  sht3_mode=SHT3.sensor_conf;
	  if(sht3_mode==normal) printf("SHTC3 normal mode\r\n");
	  else printf("SHTC3 low power mode\r\n");

	  bme280_conf_change(BME280.sensor_conf);

	  MS8607_osr(MS8607.sensor_conf);
	  printf("MS8607 OSR %d\r\n", 256<<MS8607.sensor_conf);

	  if(!TMP117.present && !SHT3.present && !MS8607.present && !BME280.present && !DPS368.present)
		  config.disp_type = 0;

	  if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS) puts("Cipher init error\r");

	  Sim80x_Init(osPriorityNormal);
	  printf("SIM868 module startup %s.\r\n", Sim80x.Status.Power ? "OK" : "FAILED");
	  // uruchomienie taska sensorów
	  osThreadDef(SensorTask, SensorsTask, osPriorityNormal, 0, 512);
	  measTaskHandle = osThreadCreate(osThread(SensorTask), NULL);
	  osDelay(10);
	  osThreadDef(GPSTask, GpsReadTask, osPriorityNormal, 0, 256);
	  gpsTaskHandle = osThreadCreate(osThread(GPSTask), NULL);
	  osDelay(10);
	  osThreadDef(SendGPRSTask, GprsSendTask, osPriorityNormal, 0, 512);
	  GprsSendTaskHandle = osThreadCreate(osThread(SendGPRSTask), NULL);
	  osDelay(10);

	  uint32_t ticks30ms = HAL_GetTick();
	  uint32_t ticksbqwd = HAL_GetTick();
	  uint32_t secdiv = HAL_GetTick();
	  uint8_t registered = 0;
	  uint8_t measint = 99;

	  const uint32_t UID = getUID();
	  GPRS_status_frame.token = GPRS_TOKEN;
	  GPRS_status_frame.UID = UID;
	  GPRS_GPS_frame.token = GPRS_TOKEN;
	  GPRS_GPS_frame.UID = UID;
	  GPRS_meas_frame.token = GPRS_TOKEN;
	  GPRS_meas_frame.UID = UID;

	  meas_start = -1;
	  meas_count = config.measures;
	  if(meas_count == 0) meas_count = 1;

	  while (1)
	  {
		  CLI();

		  // miganie LED i test wyłącznika
		  if(HAL_GetTick()-ticks30ms >= 30) {
			  ticks30ms = HAL_GetTick();
			  LED1_TOGGLE();
			  check_powerOff();
			  GsmWdt();

			  if(!registered && Sim80x.Status.RegisterdToNetwork) {
				  printf("Succesfully registered to network.\r\n");
				  registered = 1;
			  }
			  if(registered && Sim80x.Status.RegisterdToNetwork == 0) registered = 0;


			  if(meas_start-seconds < 120) sendretry = 0;			// nie ponawiaj wysylek jak mniej niz 2min do kolejnej
			  if(!meas_cont_mode && sendretry && lastSendStatus > GPRSsendStatusInprogress) {
				  if(++sendretry > RESEND_DELAY*RESEND_RETRIES) sendretry = 0;		// za duzo prob, stop
				  if(lastSendStatus == GPRSsendStatusOk) sendretry = 0;				// wyslano OK, stop
				  if(lastSendStatus == GPRSsendStatusError && (sendretry % RESEND_DELAY) == RESEND_DELAY-1) {
					  printf("GPRS Send retry %d\r\n", sendretry/RESEND_DELAY + 1);
					  printf("Starting GPRS thread %s\r\n", StartSendGPRS() ? "OK":"ERROR");	// send retry
				  }
			  }
		  }

		  // reset BQ
		  if(HAL_GetTick()-ticksbqwd >= 15000) {
			  ticksbqwd = HAL_GetTick();
			  BQ25798_WD_RST();
		  }

		  // wyslanie i wyswietlenie pomiarow
	      if(sensors_data_ready) {						// taks sensorow zakonczyl dzialanie ?
	    	  sensors_data_ready = 0;
	    	  if(config.disp_type > 0) {
	    		  display_values(config.disp_type);
	    	  }

	    	  if(send_enable) {
	    		  send_enable = 0;
	    		  if(!meas_cont_mode) {
	    			  printf("Starting GPRS thread %s\r\n", StartSendGPRS() ? "OK":"ERROR");
					  CalculateNextMeasTime();					// oblicz czas rozpoczecia kolejnego pomiaru
					  PreparePacketOutData();
					  sendretry = 1;
	    		  }
	    	  }
	      }
//--------------------------------------------------------------------------------------------------
		  // zadania wykonywane co sekunde
		  if(HAL_GetTick() - secdiv >= 1000UL) {
			  secdiv = HAL_GetTick();
			  // zwieksz globalny licznik sekund liczacy sekundy danego jednego dnia
			  if(++seconds >= SEC_PER_DAY) seconds = 0;
			  // zeruj flage nastepnego dnia (nie przy stricte 0, bo korekta moze spowodowac przeoczenie)
			  if(seconds < 60) dayleap = false;

			  // ===================================================================================

			  if(meas_start < 0 && timesync) CalculateNextMeasTime();		// oblicz czas rozpoczecia pomiaru
			  if(registered && meas_start < 0) SysTimeSync();

			  // uruchomienie pomiaru
			  if((!dayleap && meas_start >= 0 && seconds >= meas_start) || meas_cont_mode) {	// czas uruchomic pomiar ?
				  if(++measint >= 5) {
					  measint = 0;
					  if (meas_count > 0 || meas_cont_mode) {
						  if(meas_cont_mode == 0) {
							  if(meas_count == config.measures && Sim80x.Status.RegisterdToNetwork == 0) Sim80x.Status.FatalError = 1;
							  if(--meas_count == 0) {
								  printf("Last measure & Send\r\n");
								  send_enable = 1;
								  measint = 99;
								  meas_count = config.measures;
							  } else printf("Measure no.%u\r\n", config.measures-meas_count);
						  }
						  vTaskResume(measTaskHandle);						// odblokuj taks pomiarow
					  }
				  }
			  }

			  // ===================================================================================

			  // czas na odczyt GPS ?, nie odczytuj w trakcie połączenia z serwerem, bo komendy bruzdza
			  if(++gps_start >= gps_interval) {
				  if(StartReadGps()) {
					  gps_start = 0;
					  gps_interval = 20*60;						// co 20 minut proba odczytu GPS
				  } else gps_interval += 30;					// nie wolno zalaczyc GPS -> za 30sek kolejna proba
			  }
			  // jak zlapano FIX to następny odczyt GPS za 12 godzin
			  if(Sim80x.GPS.Fix && gps_interval < GPS_INTERVAL)  gps_interval = GPS_INTERVAL;	// GPS OK, nastepny raz za 12 godzin

			  // ===================================================================================
		  }
	      __WFI();
	  }
}

