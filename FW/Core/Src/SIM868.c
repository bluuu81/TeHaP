/*
 * SIM868.c
 *
 *  Created on: Mar 2, 2024
 *      Author: bluuu
 */
#include "SIM868.h"

void SIM_HW_OFF()
{
	SIM_OFF();
	GPS_OFF();
}
