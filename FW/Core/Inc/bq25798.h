/*
 * bq25798.h
 *
 *  Created on: 18 maj 2023
 *      Author: bluuu
 */

#ifndef INC_BQ25798_H_
#define INC_BQ25798_H_

extern I2C_HandleTypeDef hi2c1;

// BQ25897 registers
#define REG00_Minimal_System_Voltage	0x00
#define REG01_Charge_Voltage_Limit		0x01
#define REG03_Charge_Current_Limit		0x03
#define REG05_Input_Voltage_Limit		0x05
#define REG06_Input_Current_Limit		0x06
#define REG08_Precharge_Control			0x08
#define REG09_Termination_Control		0x09
#define REG0A_Recharge_Control			0x0A
#define REG0B_VOTG_regulation			0x0B
#define REG0D_IOTG_regulation			0x0D
#define REG0E_Timer_Control				0x0E
#define REG0F_Charger_Control_0			0x0F
#define REG10_Charger_Control_1			0x10
#define REG11_Charger_Control_2			0x11
#define REG12_Charger_Control_3			0x12
#define REG13_Charger_Control_4			0x13
#define REG14_Charger_Control_5			0x14
#define REG15_MPPT_Control				0x15
#define REG16_Temperature_Control		0x16
#define REG17_NTC_Control_0				0x17
#define REG18_NTC_Control_1				0x18
#define REG19_ICO_Current_Limit			0x19
#define REG1B_Charger_Status_0			0x1B
#define REG1C_Charger_Status_1			0x1C
#define REG1D_Charger_Status_2			0x1D
#define REG1E_Charger_Status_3			0x1E
#define REG1F_Charger_Status_4			0x1F
#define REG20_FAULT_Status_0			0x20
#define REG21_FAULT_Status_1			0x21
#define REG22_Charger_Flag_0			0x22
#define REG23_Charger_Flag_1			0x23
#define REG24_Charger_Flag_2			0x24
#define REG25_Charger_Flag_3			0x25
#define REG26_FAULT_Flag_0				0x26
#define REG27_FAULT_Flag_1				0x27
#define REG28_Charger_Mask_0			0x28
#define REG29_Charger_Mask_1			0x29
#define REG2A_Charger_Mask_2			0x2A
#define REG2B_Charger_Mask_3			0x2B
#define REG2C_FAULT_Mask_0				0x2C
#define REG2D_FAULT_Mask_1				0x2D
#define REG2E_ADC_Control				0x2E
#define REG2F_ADC_Function_Disable_0	0x2F
#define REG30_ADC_Function_Disable_1	0x30
#define REG31_IBUS_ADC					0x31
#define REG33_IBAT_ADC					0x33
#define REG35_VBUS_ADC					0x35
#define REG37_VAC1_ADC					0x37
#define REG39_VAC2_ADC					0x39
#define REG3B_VBAT_ADC					0x3B
#define REG3D_VSYS_ADC					0x3D
#define REG3F_TS_ADC					0x3F
#define REG41_TDIE_ADC					0x41
#define REG43_Dplus_ADC					0x43
#define REG45_Dminus_ADC				0x45
#define REG47_DPDM_Driver				0x47
#define REG48_Part_Information			0x48

enum charge_state
{
	FAULT = 0,
	OK,
	NO_CHARGE,
	CHARGE
};

uint8_t BQ25798_check();
void BQ25798_set_ADC();
uint16_t BQ25798_Vbat_read();
uint16_t BQ25798_Vsys_read();
uint16_t BQ25798_Vbus_read();
uint16_t BQ25798_Vac1_read();
uint16_t BQ25798_Vac2_read();
uint16_t BQ25798_Ibus_read();
uint16_t BQ25798_Ibat_read();

uint16_t BQ25798_Sys_Min_Voltage_read();
uint16_t BQ25798_Chr_Volt_Limit_read();
uint16_t BQ25798_Chr_Curr_Limit_read();
uint8_t BQ25798_Sys_Min_Voltage_write(uint8_t bits); // 6 bits multiplier (2500mV + 6bits * 250mV) e.g 3000mV = 2500 + 3*250 = 3,25V / bits=3
uint8_t BQ25798_Chr_Volt_Limit_write(uint16_t val); // mV
uint8_t BQ25798_Chr_Curr_Limit_write(uint16_t val); // mA
uint8_t BQ25798_Chr_Input_Voltage_Limit_write(uint8_t val); //*100mV
uint8_t BQ25798_Chr_Input_Curr_Limit_write(uint16_t val); //*10mA

uint8_t BQ25798_Chrg_CTRL1_write(uint8_t hex_val);
uint8_t BQ25798_Chrg_NTC_CTRL1_write(uint8_t hex_val);

uint8_t BQ25798_WD_RST();
uint8_t BQ25798_MPPT_CTRL(uint8_t set);

uint8_t BQ25798_Chrg_CTRL3_read();
uint8_t BQ25798_Chrg_CTRL4_read();

uint8_t BQ25798_Chrg_FAULT1_read();
uint8_t BQ25798_Chrg_FAULT2_read();

uint8_t BQ25798_Chrg_STAT0_read();
uint8_t BQ25798_Chrg_STAT1_read();
uint8_t BQ25798_Chrg_STAT2_read();
uint8_t BQ25798_Chrg_STAT3_read();
uint8_t BQ25798_Chrg_STAT4_read();

#endif /* INC_BQ25798_H_ */
