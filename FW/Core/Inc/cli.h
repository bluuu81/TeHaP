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
#include "main.h"
#include "Sim80xDrv.h"


#define MIN_OFFSET -10.0f
#define MAX_OFFSET +10.0f

void print_status();
void print_help();
void CLI();
void CLI_proc(char ch);
int _write(int file, char *ptr, int len);

#endif /* INC_CLI_H_ */
