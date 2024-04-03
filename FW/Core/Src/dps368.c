/*
 * dps368.c
 *
 *  Created on: 20 cze 2023
 *      Author: bluuu
 */

#include "dps368.h"
#include "thp_sensors.h"

DPS_coeff_t DPS_coef;
volatile int32_t Kt_coef, Kp_coef;

uint8_t DPS368_check()
{
	uint8_t value;
	SET_DPS368();
	HAL_StatusTypeDef status;
	status = HAL_I2C_IsDeviceReady(&hi2c2, DPS368_ADDR, 3, 150);
	osDelay(100);
	if (status == HAL_OK) {
		i2c_read8(&hi2c2, DPS368_REG_ID, &value, DPS368_ADDR);
		if(value == DPS368_ID_CHK) {printf("DPS368 OK\r\n"); return 1;} else {printf("NOT DPS368\r\n"); return 0;}
	} else {printf("DPS368 FAILED\r\n"); return 0;}
	return 0;
}

void getTwosComplement(int32_t *raw, uint8_t length)
{
    if (*raw & ((uint32_t)1 << (length - 1)))
    {
        *raw -= (uint32_t)1 << length;
    }
}


void DPS368_read_coeff()
{
	uint8_t regs[18];
	SET_DPS368();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, 0x10, I2C_MEMADD_SIZE_8BIT, regs, 18, 250);
//	for(uint8_t i=0; i<19;i++){
//		printf("Coef %d : %#x\r\n",i,regs[i]);
//	}
	DPS_coef.C0 = ((uint32_t)regs[0] << 4) | (((uint32_t)regs[1] >> 4) & 0x0F);
    getTwosComplement(&DPS_coef.C0, 12);
    // c0 is only used as c0*0.5, so c0_half is calculated immediately
    DPS_coef.C0 = DPS_coef.C0 / 2U;

    // now do the same thing for all other coefficients
    DPS_coef.C1 = (((uint32_t)regs[1] & 0x0F) << 8) | (uint32_t)regs[2];
    getTwosComplement(&DPS_coef.C1, 12);
    DPS_coef.C00 = ((uint32_t)regs[3] << 12) | ((uint32_t)regs[4] << 4) | (((uint32_t)regs[5] >> 4) & 0x0F);
    getTwosComplement(&DPS_coef.C00, 20);
    DPS_coef.C10 = (((uint32_t)regs[5] & 0x0F) << 16) | ((uint32_t)regs[6] << 8) | (uint32_t)regs[7];
    getTwosComplement(&DPS_coef.C10, 20);

    DPS_coef.C01 = ((uint32_t)regs[8] << 8) | (uint32_t)regs[9];
    getTwosComplement(&DPS_coef.C01, 16);

    DPS_coef.C11 = ((uint32_t)regs[10] << 8) | (uint32_t)regs[11];
    getTwosComplement(&DPS_coef.C11, 16);
    DPS_coef.C20 = ((uint32_t)regs[12] << 8) | (uint32_t)regs[13];
    getTwosComplement(&DPS_coef.C20, 16);
    DPS_coef.C21 = ((uint32_t)regs[14] << 8) | (uint32_t)regs[15];
    getTwosComplement(&DPS_coef.C21, 16);
    DPS_coef.C30 = ((uint32_t)regs[16] << 8) | (uint32_t)regs[17];
    getTwosComplement(&DPS_coef.C30, 16);

//    printf("C0: %ld   C1: %ld\r\n",DPS_coef.C0,DPS_coef.C1);
//    printf("C00: %ld   C01: %ld\r\n",DPS_coef.C00,DPS_coef.C01);
//    printf("C10: %ld   C11: %ld\r\n",DPS_coef.C10,DPS_coef.C11);
//    printf("C20: %ld   C21: %ld\r\n",DPS_coef.C20,DPS_coef.C21);
//    printf("C30: %ld\r\n",DPS_coef.C30);
}

void dumpCFGreg()
{
	uint8_t reg;
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
	printf("CFG REG 0x09 DUMP (hex): %#x\r\n",reg);
	printbinaryMSB(reg);
}

void DPS368_fifo(uint8_t endis)
{
	uint8_t reg;
	SET_DPS368();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
//	printf("(fifo) CFG REG READ (hex): %#x\r\n",reg);
//	printbinaryMSB(reg);
	setBit(&reg,1,endis);
//	printf("SET FIFO EN\r\n");
//	printf("(fifo) CFG REG WRITE (hex): %#x\r\n",reg);
//	printbinaryMSB(reg);
	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
	osDelay(1);
}

void DPS368_conf_int(uint8_t ints)
{
	uint8_t reg;
	SET_DPS368();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
//	printf("CFG REG (hex): %#x\r\n",reg);
//	printbinaryMSB(reg);
	modifyRegister(&reg, 0xF0, ints);
//	printf("SET INT \r\n");
//	printf("CFG REG (hex): %#x\r\n",reg);
//	printbinaryMSB(reg);
	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
	osDelay(1);
}

void DPS368_temp_source()
{
	uint8_t reg, reg_mod;
	SET_DPS368();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_TEMP_CFG, I2C_MEMADD_SIZE_8BIT, &reg_mod, 1, 250);
//	printf("CFG TEMP SRC 0x07 (hex) (NOMOD): %#x\r\n",reg_mod);
//	printbinaryMSB(reg_mod);

	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, 0x28, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
//	printf("CFG TEMP SRC 0x28 (hex): %#x\r\n",reg);
//	printbinaryMSB(reg);
	if((reg & 0x80) == 0) {
//		printf("Internal Temp\r\n");
		setBit(&reg_mod, 7, 0);
	} else {
//		printf("External Temp\r\n");
		setBit(&reg_mod, 7, 1);
		}
//	printf("CFG TEMP SRC 0x07 (hex) (MOD): %#x\r\n",reg_mod);
//	printbinaryMSB(reg_mod);

	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_TEMP_CFG, I2C_MEMADD_SIZE_8BIT, &reg_mod, 1, 250);

}


void DPS368_conf_temp(uint8_t ovr, uint8_t rate)
{
	uint8_t reg;
	SET_DPS368();
	reg = ovr + rate;
	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_TEMP_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
    switch (ovr)
    {
        case DPS_OVERSAMPLE_1:
            Kt_coef = DPS_OSR_SF_1;
            break;

        case DPS_OVERSAMPLE_2:
        	Kt_coef = DPS_OSR_SF_2;
            break;

        case DPS_OVERSAMPLE_4:
        	Kt_coef = DPS_OSR_SF_4;
            break;

        case DPS_OVERSAMPLE_8:
        	Kt_coef = DPS_OSR_SF_8;
            break;

        case DPS_OVERSAMPLE_16:
        	Kt_coef = DPS_OSR_SF_16;
            break;

        case DPS_OVERSAMPLE_32:
        	Kt_coef = DPS_OSR_SF_32;
            break;

        case DPS_OVERSAMPLE_64:
        	Kt_coef = DPS_OSR_SF_64;
            break;

        case DPS_OVERSAMPLE_128:
        	Kt_coef = DPS_OSR_SF_128;
            break;
    }
//    printf("Kt_coef set: %lu\r\n",Kt_coef);

   	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
    if(ovr > DPS_OVERSAMPLE_8) setBit(&reg, 3, 1);
    else setBit(&reg, 3, 0);
   	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);

	DPS368_temp_source();

}

void DPS368_conf_press(uint8_t ovr, uint8_t rate)
{
	uint8_t reg;
	SET_DPS368();
	reg = ovr + rate;
	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_PRESS_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
    switch (ovr)
    {
        case DPS_OVERSAMPLE_1:
            Kp_coef = DPS_OSR_SF_1;
            break;

        case DPS_OVERSAMPLE_2:
        	Kp_coef = DPS_OSR_SF_2;
            break;

        case DPS_OVERSAMPLE_4:
        	Kp_coef = DPS_OSR_SF_4;
            break;

        case DPS_OVERSAMPLE_8:
        	Kp_coef = DPS_OSR_SF_8;
            break;

        case DPS_OVERSAMPLE_16:
        	Kp_coef = DPS_OSR_SF_16;
            break;

        case DPS_OVERSAMPLE_32:
        	Kp_coef = DPS_OSR_SF_32;
            break;

        case DPS_OVERSAMPLE_64:
        	Kp_coef = DPS_OSR_SF_64;
            break;

        case DPS_OVERSAMPLE_128:
        	Kp_coef = DPS_OSR_SF_128;
            break;
    }

//    printf("Kp_coef set: %lu\r\n",Kp_coef);

   	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
    if(ovr > DPS_OVERSAMPLE_8) setBit(&reg, 2, 1);
    else setBit(&reg, 2, 0);
   	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);


}

void DPS368_clearFIFO(uint8_t mode)
{
	uint8_t reg;
	SET_DPS368();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
	setBit(&reg,7,1);
	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);

}

void DPS368_temp_correct(uint8_t ovr)
{
	SET_DPS368();
	HAL_StatusTypeDef status;
	uint8_t write_data = 0xA5;
	status = HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, 0x0E, I2C_MEMADD_SIZE_8BIT, &write_data, 1, 250);
	if (status == HAL_OK) {
        write_data = 0x96;
    	status = HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, 0x0F, I2C_MEMADD_SIZE_8BIT, &write_data, 1, 250);
	}
	if (status == HAL_OK) {
        write_data = 0x02;
    	status = HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, 0x62, I2C_MEMADD_SIZE_8BIT, &write_data, 1, 250);
	}
	if (status == HAL_OK) {
        write_data = 0x00;
    	status = HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, 0x0E, I2C_MEMADD_SIZE_8BIT, &write_data, 1, 250);
	}
	if (status == HAL_OK) {
        write_data = 0x00;
    	status = HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, 0x0F, I2C_MEMADD_SIZE_8BIT, &write_data, 1, 250);
	}
	DPS368_conf_temp(ovr, DPS_RATE_1);
	DPS368_run_mode(MODE_CMD_TEMP);

}


void DPS368_run_mode(uint8_t mode)
{
	SET_DPS368();
	uint8_t reg;
//	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_MEAS_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
//	reg = (reg & 0x0E) | mode;
	reg = mode;
	HAL_I2C_Mem_Write(&hi2c2, DPS368_ADDR, DPS368_MEAS_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
	osDelay(2);
}

uint8_t DPS368_temp_rdy()
{
	SET_DPS368();
	uint8_t reg;
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_MEAS_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
//	printf("Temp RDY: %#x\r\n", reg);
//	printbinaryMSB(reg);
	return ((reg & 0x20) >> 5);

}

uint8_t DPS368_press_rdy()
{
	SET_DPS368();
	uint8_t reg;
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_MEAS_CFG, I2C_MEMADD_SIZE_8BIT, &reg, 1, 250);
	return ((reg & 0x10) >> 4);
}

float DPS368_get_temp_cmd(uint8_t ovr)
{
	DPS368_conf_temp(ovr, 0U);
	DPS368_run_mode(MODE_CMD_TEMP);
	osDelay(calcBusyTime(ovr));
	osDelay(1);
	SET_DPS368();
	uint8_t value[3];
	int32_t raw_temp;
	float temp_scaled, temperature;
//	dumpCFGreg();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_TEMP, I2C_MEMADD_SIZE_8BIT, value, 3, 250);
	raw_temp = (int32_t)(value[2]) + (value[1] << 8) + (value[0] << 16);
	getTwosComplement(&raw_temp, 24);
	const float scaling = 1.0f/Kt_coef;
	//printf("DPS RAW VALUE: %ld\r\n", raw_temp);
//	printf("DPS SCALING VALUE: %.12f\r\n", scaling);
	temp_scaled = (float)raw_temp * scaling;
//	printf("DPS TEMP SCALED VALUE: %.3f\r\n", temp_scaled);
	temperature = DPS_coef.C0 + DPS_coef.C1 * temp_scaled;
	return temperature;
}



float DPS368_get_press_cmd(uint8_t ovr)
{
	DPS368_conf_temp(ovr, 0U);
	DPS368_run_mode(MODE_CMD_TEMP);
	osDelay(calcBusyTime(ovr));
	osDelay(1);
	SET_DPS368();
	uint8_t value[3];
	int32_t raw_temp, raw_press;
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_TEMP, I2C_MEMADD_SIZE_8BIT, value, 3, 250);
	raw_temp = (int32_t)(value[2]) + (value[1] << 8) + (value[0] << 16);
	getTwosComplement(&raw_temp, 24);
	const float scalingT = 1.0f/Kt_coef;
	printf("DPS RAW TEMP VALUE: %ld\r\n", raw_temp);
	printf("DPS SCALING TEMP VALUE: %.12f\r\n", scalingT);
	float temp_scaled = (float)raw_temp * scalingT;
	printf("DPS TEMP SCALED VALUE: %.3f\r\n", temp_scaled);

	float press_scaled, pressure;
	DPS368_conf_press(ovr, 0U);
	DPS368_run_mode(MODE_CMD_PRESS);
	osDelay(calcBusyTime(ovr));
	osDelay(1);
	SET_DPS368();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_PRESS, I2C_MEMADD_SIZE_8BIT, value, 3, 250);
	raw_press = (int32_t)(value[2]) + (value[1] << 8) + (value[0] << 16);
	getTwosComplement(&raw_press, 24);
	const float scalingP = 1.0f/Kp_coef;
	printf("DPS RAW PRESS VALUE: %ld\r\n", raw_press);
	printf("DPS SCALING PRESS VALUE: %.12f\r\n", scalingP);
	press_scaled = (float)raw_press * scalingP;
	printf("DPS PRESS SCALED VALUE: %.3f\r\n", press_scaled);
    pressure = DPS_coef.C00;
	pressure += press_scaled * (DPS_coef.C10 + press_scaled * (DPS_coef.C20 + press_scaled * DPS_coef.C30));
	pressure += (temp_scaled * DPS_coef.C01);
	pressure += (temp_scaled * press_scaled * (DPS_coef.C11 + press_scaled * DPS_coef.C21));
	return pressure *0.01f;
}



uint32_t calcBusyTime(uint8_t osr)
{
    // formula from datasheet (optimized)
    return (((uint32_t)20U) + ((uint32_t)16U << ((uint16_t)osr)));
}

void DPS368_init(uint8_t fifo, uint8_t int_m)
{
	DPS368_read_coeff();
	DPS368_conf_int(int_m);
	DPS368_fifo(fifo);
	DPS368_run_mode(MODE_IDLE);
}

void DPS368_start_meas_temp(uint8_t ovr)
{
	DPS368_conf_temp(ovr, 0U);
	DPS368_run_mode(MODE_CMD_TEMP);
}

float DPS368_get_temp()
{
	SET_DPS368();
	uint8_t value[3];
	int32_t raw_temp;
	float temp_scaled, temperature;
//	dumpCFGreg();
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_TEMP, I2C_MEMADD_SIZE_8BIT, value, 3, 250);
	raw_temp = (int32_t)(value[2]) + (value[1] << 8) + (value[0] << 16);
	getTwosComplement(&raw_temp, 24);
	const float scaling = 1.0f/Kt_coef;
	//printf("DPS RAW VALUE: %ld\r\n", raw_temp);
//	printf("DPS SCALING VALUE: %.12f\r\n", scaling);
	temp_scaled = (float)raw_temp * scaling;
//	printf("DPS TEMP SCALED VALUE: %.3f\r\n", temp_scaled);
	temperature = DPS_coef.C0 + DPS_coef.C1 * temp_scaled;
	return temperature;
}

float DPS368_calc_temp(float temp_scaled)
{
	float temperature;
//	temp_scaled = DPS368_get_scaled_temp();
//	printf("DPS TEMP SCALED VALUE: %.3f\r\n", temp_scaled);
	temperature = DPS_coef.C0 + DPS_coef.C1 * temp_scaled;
	return temperature;
}

void DPS368_start_meas_press(uint8_t ovr)
{
	DPS368_conf_press(ovr, 0U);
	DPS368_run_mode(MODE_CMD_PRESS);
}

float DPS368_get_scaled_temp()
{
	SET_DPS368();
	uint8_t value[3];
	int32_t raw_temp;
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_TEMP, I2C_MEMADD_SIZE_8BIT, value, 3, 250);
	raw_temp = (int32_t)(value[2]) + (value[1] << 8) + (value[0] << 16);
	getTwosComplement(&raw_temp, 24);
	const float scalingT = 1.0f/Kt_coef;
	return ((float)raw_temp * scalingT);
}

float DPS368_get_press(float temp_scaled)
{
	SET_DPS368();
	uint8_t value[3];
	int32_t raw_press;
	HAL_I2C_Mem_Read(&hi2c2, DPS368_ADDR, DPS368_PRESS, I2C_MEMADD_SIZE_8BIT, value, 3, 250);
	raw_press = (int32_t)(value[2]) + (value[1] << 8) + (value[0] << 16);
	getTwosComplement(&raw_press, 24);
	const float scalingP = 1.0f/Kp_coef;
//	printf("DPS RAW PRESS VALUE: %ld\r\n", raw_press);
//	printf("DPS SCALING PRESS VALUE: %.12f\r\n", scalingP);
	float press_scaled, pressure;
	press_scaled = (float)raw_press * scalingP;
//	printf("DPS PRESS SCALED VALUE: %.3f\r\n", press_scaled);
    pressure = DPS_coef.C00;
	pressure += press_scaled * (DPS_coef.C10 + press_scaled * (DPS_coef.C20 + press_scaled * DPS_coef.C30));
	pressure += (temp_scaled * DPS_coef.C01);
	pressure += (temp_scaled * press_scaled * (DPS_coef.C11 + press_scaled * DPS_coef.C21));
//	return pressure *0.01f;
	return pressure;
}

uint16_t dps368_ovr_config(uint8_t sensor_conf)
{
    switch (sensor_conf) {
        case 0:
        	printf("DPS368 set TEMP & PRESS Oversample x1\r\n");
            return combine_uint8(DPS_OVERSAMPLE_1,DPS_OVERSAMPLE_1);
        case 1:
        	printf("DPS368 set TEMP & PRESS Oversample x2\r\n");
            return combine_uint8(DPS_OVERSAMPLE_2,DPS_OVERSAMPLE_2);
        case 2:
        	printf("DPS368 set TEMP & PRESS Oversample x4\r\n");
            return combine_uint8(DPS_OVERSAMPLE_4,DPS_OVERSAMPLE_4);
        case 3:
        	printf("DPS368 set TEMP & PRESS Oversample x8\r\n");
            return combine_uint8(DPS_OVERSAMPLE_8,DPS_OVERSAMPLE_8);
        case 4:
        	printf("DPS368 set TEMP & PRESS Oversample x16\r\n");
            return combine_uint8(DPS_OVERSAMPLE_16,DPS_OVERSAMPLE_16);
        case 5:
        	printf("DPS368 set TEMP & PRESS Oversample x32\r\n");
            return combine_uint8(DPS_OVERSAMPLE_32,DPS_OVERSAMPLE_32);
        case 6:
        	printf("DPS368 set TEMP & PRESS Oversample x64\r\n");
            return combine_uint8(DPS_OVERSAMPLE_64,DPS_OVERSAMPLE_64);
        case 7:
        	printf("DPS368 set TEMP & PRESS Oversample x128\r\n");
            return combine_uint8(DPS_OVERSAMPLE_128,DPS_OVERSAMPLE_128);
        case 8:
        	printf("DPS368 set TEMP Oversample x128, PRESS Oversample x4 \r\n");
            return combine_uint8(DPS_OVERSAMPLE_128,DPS_OVERSAMPLE_4);

        default:
        	printf("DPS368 set TEMP Oversample x128, PRESS Oversample x4 \r\n");
            return combine_uint8(DPS_OVERSAMPLE_128,DPS_OVERSAMPLE_4);
    }
}
