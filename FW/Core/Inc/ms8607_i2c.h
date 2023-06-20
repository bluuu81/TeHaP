/*
 * ms8607_i2c.h
 *
 *  Created on: 4 cze 2023
 *      Author: bluuu
 */

#ifndef INC_MS8607_I2C_H_
#define INC_MS8607_I2C_H_

extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
#include "main.h"

enum i2c_transfer_direction {
 	I2C_TRANSFER_WRITE = 0,
 	I2C_TRANSFER_READ  = 1,
};

enum status_code {
 	STATUS_OK           = 0x00,
 	STATUS_ERR_OVERFLOW	= 0x01,
	STATUS_ERR_TIMEOUT  = 0x02,
};

struct i2c_master_packet {
	// Address to slave device
 	uint16_t address;
 	// Length of data array
 	uint16_t data_length;
 	// Data array containing all data to be transferred
 	uint8_t *data;
};

void i2c_master_init(void);
enum status_code i2c_master_read_packet_wait(struct i2c_master_packet *const packet);
enum status_code i2c_master_write_packet_wait(struct i2c_master_packet *const packet);
enum status_code i2c_master_write_packet_wait_no_stop(struct i2c_master_packet *const packet);

#endif /* INC_MS8607_I2C_H_ */
