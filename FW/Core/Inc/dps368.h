/*
 * dps368.h
 *
 *  Created on: 20 cze 2023
 *      Author: bluuu
 */

#ifndef INC_DPS368_H_
#define INC_DPS368_H_

#include "main.h"
#include "thp_sensors.h"

#define DPS368_ADDR				0x76 << 1
#define DPS368_REG_ID			0x0D
#define DPS368_ID_CHK			0x10

uint8_t DPS368_check();


#endif /* INC_DPS368_H_ */
