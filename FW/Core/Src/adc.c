/*
 * adc.c
 *
 *  Created on: May 25, 2023
 *      Author: bluuu
 */


#include "main.h"
#include "adc.h"
#include "thp.h"
#include <stdio.h>

volatile uint16_t adc_data[5];


void ADC_Print()
{
	  printf("ADC 1: %d , ADC 2: %d ADC 3: %d ADC 4: %d ADC 5: %d \r\n", adc_data[0], adc_data[1], adc_data[2], adc_data[3], adc_data[4]);
}


void ADC_DMA_Start()
{
	  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);                    // ADC calibration
	  HAL_Delay(10);
	  HAL_ADC_Start_DMA(&hadc1, (uint32_t *) adc_data, 5);   // start ADC DMA (1 channel, 5 reads per channel)
}

float GET_MCU_Temp()
{
	    uint16_t tmp=0;
	    float temperature;
	    uint16_t cal_temp = *((uint16_t*) ((uint32_t)0x1FFF75A8));
   	    uint16_t cal_value = *((uint16_t*) ((uint32_t)0x1FFF75CA));

	  	for(uint8_t i=0; i<5;i+=1)
	  	{
	  		tmp+=adc_data[i];
	  	}
	  	tmp*=0.2f;

	  	temperature = ((cal_temp - (float)tmp) * 4.3) / cal_value + 25.0;
	  	return temperature;
}


