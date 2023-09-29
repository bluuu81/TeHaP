/*
 * cli.h
 *
 *  Created on: May 16, 2023
 *      Author: bluuu
 */

#ifndef INC_CLI_H_
#define INC_CLI_H_

#include "stm32l4xx_hal.h"
#include "thp_sensors.h"

extern TEMP_struct_t TMP117_temp_sensor;
extern TEMP_struct_t MS8607_temp_sensor;
extern TEMP_struct_t SHTC3_temp_sensor;

#define MIN_OFFSET -10.0
#define MAX_OFFSET +10.0

void help();
void CLI();
void CLI_proc(char ch);

#endif /* INC_CLI_H_ */
