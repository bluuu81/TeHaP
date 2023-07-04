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
#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define DPS368_ADDR				0x76 << 1
#define DPS368_REG_ID			0x0D
#define DPS368_ID_CHK			0x10

#define DPS368_TEMP				0x03
#define DPS368_PRESS			0x00

#define DPS368_PRESS_CFG        0x06
#define DPS368_TEMP_CFG         0x07
#define DPS368_MEAS_CFG         0x08
#define DPS368_CFG              0x09

#define FIFO_EN					1
#define FIFO_DIS				0



typedef struct
{
    int32_t C0;       /**< 12bit temperature calibration coefficient read from sensor */
    int32_t C1;       /**< 12bit temperature calibration coefficient read from sensor */
    int32_t C00;      /**< 12bit pressure calibration coefficient read from sensor */
    int32_t C10;      /**< 20bit pressure calibration coefficient read from sensor */
    int32_t C01;      /**< 20bit pressure calibration coefficient read from sensor */
    int32_t C11;      /**< 20bit pressure calibration coefficient read from sensor */
    int32_t C20;      /**< 20bit pressure calibration coefficient read from sensor */
    int32_t C21;      /**< 20bit pressure calibration coefficient read from sensor */
    int32_t C30;      /**< 20bit pressure calibration coefficient read from sensor */
} DPS_coeff_t;

typedef enum
{
    MODE_IDLE                   =  0x00,  /**< Idle mode value */
    MODE_CMD_PRESS       		=  0x01,  /**< Single shot pressure only. After a
                                             single measurement the device will
                                             return to IDLE. Subsequent measurements
                                             must call \ref xensiv_dps3xx_set_config
                                             to set the mode again. */
    MODE_CMD_TEMP			    =  0x02,  /**< Single shot temperature only. After a
                                             single measurement the device will
                                             return to IDLE. Subsequent measurements
                                             must call \ref xensiv_dps3xx_set_config
                                             to set the mode again. */
    MODE_BACKGND_PRESS	    	=  0x05,  /**< Ongoing background pressure only */
    MODE_BACKGND_TEMP			=  0x06,  /**< Ongoing background temperature only */
    MODE_BACKGND_ALL			=  0x07   /**< Ongoing background pressure and temp */
} DPS368_mode;

/** Enum to enable/disable/report interrupt cause flags. */
typedef enum
{
    INT_NONE  = 0x00, /**< No interrupt enabled */
    INT_HL    = 0x80, /**< Interrupt (on SDO pin) active level */
    INT_FIFO  = 0x40, /**< Interrupt when the FIFO is full */
    INT_TMP   = 0x20, /**< Interrupt when a temperature measurement is ready */
    INT_PRS   = 0x10  /**< Interrupt when a pressure measurement is ready */
} DPS368_int;

// Values here correspond to register field values
/** Oversampling rates for both pressure and temperature measurements */
typedef enum
{
    DPS_OVERSAMPLE_1   = 0x00, /**< 1x oversample rate */
    DPS_OVERSAMPLE_2   = 0x01, /**< 2x oversample rate */
    DPS_OVERSAMPLE_4   = 0x02, /**< 4x oversample rate */
    DPS_OVERSAMPLE_8   = 0x03, /**< 8x oversample rate */
    DPS_OVERSAMPLE_16  = 0x04, /**< 16x oversample rate */
    DPS_OVERSAMPLE_32  = 0x05, /**< 32x oversample rate */
    DPS_OVERSAMPLE_64  = 0x06, /**< 64x oversample rate */
    DPS_OVERSAMPLE_128 = 0x07  /**< 128x oversample rate */
} DPS368_oversample;

// Values here correspond to register field values
/** Measurement rates for both pressure and temperature measurements */
typedef enum
{
    DPS_RATE_1   = 0x00, /**< Normal sample rate */
	DPS_RATE_2   = 0x10, /**< 2x sample rate */
    DPS_RATE_4   = 0x20, /**< 4x sample rate */
    DPS_RATE_8   = 0x30, /**< 8x sample rate */
    DPS_RATE_16  = 0x40, /**< 16x sample rate */
    DPS_RATE_32  = 0x50, /**< 32x sample rate */
    DPS_RATE_64  = 0x60, /**< 64x sample rate */
    DPS_RATE_128 = 0x70  /**< 128x sample rate */
} DPS368_rate;

/** \cond INTERNAL */
// Values here correspond to register field values
/** Scaling coefficients for both Kp and Kt */
typedef enum
{
    DPS_OSR_SF_1   = 524288,  /**< sensor */
    DPS_OSR_SF_2   = 1572864, /**< sensor */
    DPS_OSR_SF_4   = 3670016, /**< sensor */
    DPS_OSR_SF_8   = 7864320, /**< sensor */
    DPS_OSR_SF_16  = 253952,  /**< sensor */
    DPS_OSR_SF_32  = 516096,  /**< sensor */
    DPS_OSR_SF_64  = 1040384, /**< sensor */
    DPS_OSR_SF_128 = 2088960  /**< sensor */
} DPS368_scaling_coeffs;


uint8_t DPS368_check();
void DPS368_read_coeff();
void DPS368_fifo(uint8_t mode);
void DPS368_conf_int(uint8_t ints);
void DPS368_temp_source();
void DPS368_conf_temp(uint8_t ovr, uint8_t rate);
void DPS368_conf_press(uint8_t ovr, uint8_t rate);
void DPS368_temp_correct();
void DPS368_run_mode(uint8_t mode);
uint8_t DPS368_temp_rdy();
uint8_t DPS368_press_rdy();
float DPS368_get_temp_cmd(uint8_t ovr);
float DPS368_get_press_cmd(uint8_t ovr);
void DPS368_init(uint8_t fifo, uint8_t int_m);
uint32_t calcBusyTime(uint8_t ovr);
#endif /* INC_DPS368_H_ */
