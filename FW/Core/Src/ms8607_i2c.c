/*
 * ms8607_i2c.c
 *
 *  Created on: 4 cze 2023
 *      Author: bluuu
 */

#include "ms8607.h"
#include "ms8607_i2c.h"

void i2c_master_init(void)
{
}

enum status_code i2c_master_read_packet_wait(struct i2c_master_packet *const packet)
{
    uint8_t res = HAL_I2C_Master_Receive(&hi2c2, packet->address, packet->data, packet->data_length, 20);
    return (enum status_code)res;
}


enum status_code i2c_master_write_packet_wait(struct i2c_master_packet *const packet)
{
    uint8_t res = HAL_I2C_Master_Transmit(&hi2c2, packet->address, packet->data, packet->data_length, 20);
    return (enum status_code)res;
}

enum status_code i2c_master_write_packet_wait_no_stop(struct i2c_master_packet *const packet)
{
    uint8_t res = HAL_I2C_Master_Transmit(&hi2c2, packet->address, packet->data, packet->data_length, 20);
    return (enum status_code)res;
}
