/**
******************************************************************************

* SIM868 configuration
* Author: Romuald Bialy

******************************************************************************
**/
#ifndef	_SIM80XCONF_H
#define	_SIM80XCONF_H

#define GSM_UART	huart2
extern UART_HandleTypeDef huart2;


//	0: No DEBUG				1:High Level Debug .Use printf		2:All RX Data.Use printf

#define	_SIM80X_DEBUG				1
#define	_SIM80X_USART				GSM_UART
#define	_SIM80X_USE_POWER_KEY   	1
#define	_SIM80X_BUFFER_SIZE			768
#define _SIM80X_DMA_TRANSMIT        0
#define _SIM80X_USE_GPRS            1
#define _SIM80X_USE_GPS             1
#define	_SIM80X_POWER_KEY_GPIO		SIM_PWR_GPIO_Port
#define	_SIM80X_POWER_KEY_PIN		SIM_PWR_Pin

#endif
