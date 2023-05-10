/*
 * thp_sensors.c
 *
 *  Created on: 8 sty 2023
 *      Author: bluuu
 */
#include "thp_sensors.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

uint8_t i2c_read16(uint16_t offset, uint16_t *value, uint8_t addr)
{
	uint16_t tmp;
    uint8_t res = HAL_I2C_Mem_Read(&hi2c1, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 2, 8);
    *value = tmp;
    return res;
}

uint8_t i2c_write16(uint16_t offset, uint16_t value, uint8_t addr)
{
	uint16_t tmp = value;
    uint8_t res = HAL_I2C_Mem_Write(&hi2c1, addr, offset, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&tmp, 2, 8);
    return res;
}

void clr_bit(unsigned char sub_address, unsigned short new_word, uint8_t addr) {
    unsigned short old_word;
    i2c_read16(sub_address, &old_word, addr);
    old_word &= ~new_word;
    i2c_write16(sub_address, old_word, addr);
}

void set_bit(unsigned char sub_address, unsigned short new_word, uint8_t addr) {
    unsigned short old_word;
    i2c_read16(sub_address, &old_word, addr);
    old_word |= new_word;
    i2c_write16(sub_address, old_word, addr);
}
