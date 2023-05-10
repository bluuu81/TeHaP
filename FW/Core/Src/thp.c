/*
 * thp.c
 *
 *  Created on: May 9, 2023
 *      Author: bluuu
 */

#include "main.h"
#include "thp.h"

uint16_t led2_tim, led2_cycles;

static const uint8_t bri_corr[]= {
   0, 1, 2, 3, 4, 5, 7, 9, 12, 15, 18, 22, 27, 32, 38, 44, 51, 58,
   67, 76, 86, 96, 108, 120, 134, 148, 163, 180, 197, 216, 235, 255 };


void HAL_SYSTICK_Callback(void)
{
	static uint32_t led2swp, led2lev;

     if(led2_tim && ++led2swp >= led2_tim)
     {
         led2swp = 0;
         if(++led2lev >= 64 + (led2_cycles>>16))
         {
             led2lev = 0;
             led2_cycles--;
             if((led2_cycles & 0xFFFF) == 0) led2_tim = 0;
         }
         if(led2lev>=64) setLed2(0); else setLed2((led2lev<32) ? led2lev : 63-led2lev);
     } else if(led2_tim == 0) {led2swp=0; led2lev=0;}
}


void setPwmLed(uint8_t pwm, uint32_t channel)
{
    TIM_OC_InitTypeDef sConfig;
    memset(&sConfig, 0, sizeof(sConfig));
    sConfig.OCMode = (pwm==0) ? TIM_OCMODE_FORCED_INACTIVE : TIM_OCMODE_PWM1;
    sConfig.Pulse = pwm;
    HAL_TIM_OC_ConfigChannel(&htim16, &sConfig, channel);
    HAL_TIM_PWM_Start(&htim16, channel);
}

void setLed2(uint8_t bri)
{
    setPwmLed(bri_corr[bri], TIM_CHANNEL_1);
}

void ledSweepPwr(uint16_t spd, uint16_t cnt, uint16_t wait)
{
    led2_tim = spd;
    led2_cycles = cnt | (wait<<16);
}

void PWM_Init_Timers()
{
	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
}
