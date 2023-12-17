/*
 * thp.c
 *
 *  Created on: May 9, 2023
 *      Author: bluuu
 */

#include "main.h"
#include "thp.h"
#include "bq25798.h"

uint16_t led2_tim;
uint32_t led2_cycles;

static const uint8_t bri_corr[]= {
   0, 1, 2, 3, 4, 5, 7, 9, 12, 15, 18, 22, 27, 32, 38, 44, 51, 58,
   67, 76, 86, 96, 108, 120, 134, 148, 163, 180, 197, 216, 235, 255 };

volatile uint8_t device_state = 0;
volatile uint32_t offTim;



void HAL_SYSTICK_Callback(void)
{
	static uint32_t led2swp, led2lev;

    if(led2_tim && ++led2swp >= led2_tim)
    {
        led2swp = 0;
        if(++led2lev >= 64 + (led2_cycles>>16))
        {
            led2lev = 0;
            if((led2_cycles & 0xFFFF) != 0xFFFF) led2_cycles--;
            if((led2_cycles & 0xFFFF) == 0) led2_tim = 0;
        }
        if(led2lev>=64) setLed2(0); else setLed2((led2lev<32) ? led2lev : 63-led2lev);
     } else if(led2_tim == 0) {led2swp=0; led2lev=0;}

}


void setPwmLed(uint8_t pwm)
{
	__HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, pwm);
}

void setLed2(uint8_t bri)
{
    setPwmLed(bri_corr[bri]);
}

void led2Sweep(uint16_t spd, uint16_t cnt, uint16_t wait)
{
    led2_tim = spd;
    led2_cycles = cnt | (wait<<16);
}

void check_powerOn()
{
	  POWER_OFF();
	  printf("1. Check Power ON\r\n");
	  uint32_t timon = HAL_GetTick();
	  while(Power_SW_READ() == GPIO_PIN_SET)
	  {
//		  printf("2. Check Power BUT\r\n");
	    if(HAL_GetTick() - timon > 1000)     // 1 sec pushing
	    {
	    	timon = HAL_GetTick();
	        POWER_ON();    // pull-up power supply
	    	printf("Power ON\r\n");
	        break;                // break while loop
	    }
	  }
}

void check_powerOff()
{
  if(Power_SW_READ()) //power button pressed
  {
     if(offTim && HAL_GetTick() - offTim > 2000)    // 2 sec pressed
     {
    	 printf("Power off\r\n");
    	 POWER_OFF();
    	 HAL_Delay(3000);
    	 LED1_OFF();
    	 LED2_OFF();
     }
  } else offTim = HAL_GetTick();   // button released, update offTim

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

void thp_loop()
{
	CLI();

}

uint8_t HALcalculateCRC(uint8_t* data, uint8_t len)
{
    HAL_CRC_Init(&hcrc);
    __HAL_CRC_DR_RESET(&hcrc);
    uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)data, len);
    return (uint8_t)(crc & 0xFF);
}

uint8_t calculateCRC(uint8_t data[], uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

void display_values (uint8_t format)
{
	switch (format)
	{
	case 1:
		printf("-----------------------\r\n");
		printf("Temperature:\r\n");
		if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("TMP117: %.2f   ", TMP117.temp.value);
		if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("BME280: %.2f   ", BME280.temp.value);
		if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("SHTC3: %.2f   ", SHT3.temp.value);
		if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("MS8607: %.2f   ", MS8607.temp.value);
		if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("DPS368: %.2f   ", DPS368.temp.value);
		printf("\r\n-----------------------\r\n");
		printf("Press:\r\n");
		if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("BME280: %.2f   ", BME280.press.value);
		if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("MS8607: %.2f   ", MS8607.press.value);
		if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("DPS368: %.2f   ", DPS368.press.value);
		printf("\r\n-----------------------\r\n");
		printf("Hum:\r\n");
		if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("BME280: %.2f   ", BME280.hum.value);
		if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("SHTC3: %.2f   ", SHT3.hum.value);
		if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("MS8607: %.2f   ", MS8607.hum.value);
		printf("\r\n-----------------------\r\n");
		break;

	case 2:
		break;

	default:
		break;
	}

}
