/*
 * bme280.h
 *
 *  Created on: 18 cze 2023
 *      Author: bluuu
 */

#ifndef INC_BME280_H_
#define INC_BME280_H_

#include "main.h"
#include "thp_sensors.h"

#define BME280_ADDR				0x76 << 1
#define BME280_REG_ID			0xD0
#define BME280_ID_CHK			0x60

//uint8_t BME280_check();

#endif /* INC_BME280_H_ */
