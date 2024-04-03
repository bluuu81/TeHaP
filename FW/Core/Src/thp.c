/*
 * thp.c
 *
 *  Created on: May 9, 2023
 *      Author: bluuu & Mis
 */

#include "main.h"
#include "thp.h"
#include "bq25798.h"
#include "cmsis_os.h"

volatile uint8_t charger_state;

TMP117_struct_t TMP117;
SHT3_struct_t SHT3;
MS8607_struct_t MS8607;
BME280_struct_t BME280;
DPS368_struct_t DPS368;

BMP280_HandleTypedef bmp280;

Config_TypeDef config;
volatile uint16_t new_tim_interval; //w sekundach
volatile uint16_t tim_interval = 0;
volatile uint8_t disp_type;
volatile uint8_t meas_start = 0;
uint8_t meas_ready = 0;
uint16_t meas_count = 10;
uint8_t meas_cont_mode = 1;
uint8_t sensors_data_ready;

uint16_t tmp117_avr;
volatile uint8_t dps368_ovr_temp;
volatile uint8_t dps368_ovr_press;
volatile uint16_t dps368_ovr_conf;
uint8_t sht3_mode;

volatile uint8_t device_state = 0;
volatile uint32_t offTim;

osThreadId measTaskHandle;
uint32_t ticksstart;

//void _close(void) {}
//void _lseek(void) {}
void _read(void)  {}
//void _kill(void)  {}
//void _isatty(void) {}
//void _getpid(void) {}
//void _fstat(void) {}

//volatile uint32_t tim_secdiv, tim_meas;

void ReinitTimer(uint16_t interval)
{
	ticksstart = HAL_GetTick();
}

void HAL_SYSTICK_Callback(void)
{
}


void check_powerOn()
{
	uint32_t timon = HAL_GetTick();
	  while(Power_SW_READ() == GPIO_PIN_SET)
	  {
//		  printf("2. Check Power BUT\r\n");
	    if(HAL_GetTick() - timon > 1000)     // 1 sec pushing
	    {
	    	timon = HAL_GetTick();
	        POWER_ON();    // pull-up power supply
	    	printf("Power ON\r\n");
	        break;                // break while loop
	    }
	  }
}

void check_powerOff()
{
  static uint8_t keystate;
  if(Power_SW_READ()) //power button pressed
  {
	 LED2_ON();
	 keystate = 1;
     if(offTim && HAL_GetTick() - offTim > 2000)    // 2 sec pressed
     {
    	 printf("Power off\r\n");
    	 LED2_OFF();
    	 LED1_OFF();
    	 POWER_OFF();
    	 osDelay(2000);
    	 HAL_NVIC_SystemReset();
     }
  } else {
	  offTim = HAL_GetTick();   // button released, update offTim
	  if(keystate) LED2_OFF();
	  keystate = 0;
  }

}

void MCUgoSleep()
{
    // prepare wakeup pin
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2);
    // go sleep
    printf("Sleep\r\n");
    POWER_OFF();
    HAL_PWR_EnterSTANDBYMode();
}

uint8_t HALcalculateCRC(uint8_t* data, uint8_t len)
{
    HAL_CRC_Init(&hcrc);
    __HAL_CRC_DR_RESET(&hcrc);
    uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)data, len);
    return (uint8_t)(crc & 0xFF);
}
/*
uint8_t calculateCRC(uint8_t data[], uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = crc << 1;
        }
    }
    return crc;
}
*/

void printCSVheader()
{
	printf("\r\n");
	printf("CNT;");
	if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("TEMP_TMP117;");
	if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("TEMP_BME280;");
	if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("TEMP_SHTC3;");
	if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("TEMP_MS8607;");
	if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("TEMP_DPS368;");

	if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("PRESS_BME280;");
	if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("PRESS_MS8607;");
	if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("PRESS_DPS368;");

	if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("HUM_BME280;");
	if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("HUM_SHTC3;");
	if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("HUM_MS8607;");
	printf("\r\n");

}

void display_values (uint8_t format)
{
	switch (format)
	{
	case 1:
		printf("-----------------------\r\n");
		printf("Temperature:\r\n");
		if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("TMP117: %.2f   ", TMP117.temp.value+TMP117.temp.offset);
		if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("BME280: %.2f   ", BME280.temp.value+BME280.temp.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("SHTC3: %.2f   ", SHT3.temp.value+SHT3.temp.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("MS8607: %.2f   ", MS8607.temp.value+MS8607.temp.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("DPS368: %.2f   ", DPS368.temp.value+DPS368.temp.offset);
		printf("\r\n-----------------------\r\n");
		printf("Press:\r\n");
		if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("BME280: %.2f   ", BME280.press.value+BME280.press.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("MS8607: %.2f   ", MS8607.press.value+MS8607.press.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("DPS368: %.2f   ", DPS368.press.value+DPS368.press.offset);
		printf("\r\n-----------------------\r\n");
		printf("Hum:\r\n");
		if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("BME280: %.2f   ", BME280.hum.value+BME280.hum.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("SHTC3: %.2f   ", SHT3.hum.value+SHT3.hum.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("MS8607: %.2f   ", MS8607.hum.value+MS8607.hum.offset);
		printf("\r\n-----------------------\r\n");
		break;

	case 2:
		printf("%u;",++csvcnt);
		if(TMP117.present && TMP117.sensor_use && TMP117.temp.use_meas) printf("%.2f;",TMP117.temp.value+TMP117.temp.offset);
		if(BME280.present && BME280.sensor_use && BME280.temp.use_meas) printf("%.2f;",BME280.temp.value+BME280.temp.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.temp.use_meas) printf("%.2f;",SHT3.temp.value+SHT3.temp.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.temp.use_meas) printf("%.2f;",MS8607.temp.value+MS8607.temp.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.temp.use_meas) printf("%.2f;",DPS368.temp.value+DPS368.temp.offset);

		if(BME280.present && BME280.sensor_use && BME280.press.use_meas) printf("%.2f;",BME280.press.value+BME280.press.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.press.use_meas) printf("%.2f;",MS8607.press.value+MS8607.press.offset);
		if(DPS368.present && DPS368.sensor_use && DPS368.press.use_meas) printf("%.2f;",DPS368.press.value+DPS368.press.offset);

		if(BME280.present && BME280.sensor_use && BME280.hum.use_meas) printf("%.2f;",BME280.hum.value+BME280.hum.offset);
		if(SHT3.present && SHT3.sensor_use && SHT3.hum.use_meas) printf("%.2f;",SHT3.hum.value+SHT3.hum.offset);
		if(MS8607.present && MS8607.sensor_use && MS8607.hum.use_meas) printf("%.2f;",MS8607.hum.value+MS8607.hum.offset);
		printf("\r\n");
		break;

	default:
		break;
	}

}

void getConfVars()
{
	  TMP117.sensor_use = config.TMP117_use;
	  SHT3.sensor_use = config.SHT3_use;
	  MS8607.sensor_use = config.MS8607_use;
	  BME280.sensor_use = config.BME280_use;
	  DPS368.sensor_use = config.DPS368_use;

	  TMP117.sensor_conf = config.TMP117_conf;
	  TMP117.temp.use_meas = config.TMP117_t_use;
	  TMP117.temp.offset = config.TMP117_t_offset;

	  BME280.sensor_conf = config.BME280_conf;
	  BME280.temp.use_meas = config.BME280_t_use;
	  BME280.temp.offset = config.BME280_t_offset;

	  SHT3.sensor_conf = config.SHT3_conf;
	  SHT3.temp.use_meas = config.SHT3_t_use;
	  SHT3.temp.offset = config.SHT3_t_offset;

	  MS8607.sensor_conf = config.MS8607_conf;
	  MS8607.temp.use_meas = config.MS8607_t_use;
	  MS8607.temp.offset = config.MS8607_t_offset;

	  DPS368.sensor_conf = config.DPS368_conf;
	  DPS368.temp.use_meas = config.DPS368_t_use;
	  DPS368.temp.offset = config.DPS368_t_offset;

	  BME280.press.use_meas = config.BME280_p_use;
	  BME280.press.offset = config.BME280_p_offset;
	  MS8607.press.use_meas = config.MS8607_p_use;
	  MS8607.press.offset = config.MS8607_p_offset;
	  DPS368.press.use_meas = config.DPS368_p_use;
	  DPS368.press.offset = config.DPS368_p_offset;

	  BME280.hum.use_meas = config.BME280_h_use;
	  BME280.hum.offset = config.BME280_h_offset;
	  SHT3.hum.use_meas = config.SHT3_h_use;
	  SHT3.hum.offset = config.SHT3_h_offset;
	  MS8607.hum.use_meas = config.MS8607_h_use;
	  MS8607.hum.offset = config.MS8607_h_offset;
}

// ******************************************************************************************************

void SensorsTask(void const *argument)
{
	uint8_t shtc3_values[6];

	printf("Sensors task created\r\n\r\n\r\n");
	while(1)
	{
		vTaskSuspend(NULL);		// zatrzymaj taki i czekaj na komende start
		LED2_ON();				// mrugniecie czerwona
		uint32_t meas_time = 0;
		uint8_t dps368_press = 0;

		// uruchomienie pomiaru w poszczegolnych czujnikach
		if(TMP117.present){
		  if(TMP117.sensor_use && TMP117.temp.use_meas) {
			  TMP117_start_meas(tmp117_avr);
			  if(meas_time < 200) meas_time = 200;
		  }
		}
		if(BME280.present){
		  if(BME280.sensor_use && (BME280.temp.use_meas || BME280.press.use_meas || BME280.hum.use_meas) ) {
			  BME280_start_meas();
			  if(meas_time < 500) meas_time = 500;
		  }
		}
		if(SHT3.present){
		  if(SHT3.sensor_use && (SHT3.temp.use_meas || SHT3.hum.use_meas)) {
			  SHTC3_start_meas(sht3_mode);
			  if(meas_time < 100) meas_time = 100;
		  }
		}
		if(DPS368.present){
		  if(DPS368.sensor_use && (DPS368.temp.use_meas || DPS368.press.use_meas)) {
			  DPS368_start_meas_temp(dps368_ovr_temp);
			  uint32_t dpstim = calcBusyTime(dps368_ovr_temp);
			  if(meas_time < dpstim) meas_time = dpstim;
			  if(DPS368.press.use_meas) dps368_press = 1;				// z DPS bedzie tez cisnienie
		  }
		}
		if(disp_type == 1) {
		  printf("Komenda startu pomiarow wyslana\r\n");
		  printf("Meas interval: %u\r\n", tim_interval);
		}

		LED2_OFF();						// mrugniecie czerwona
		osDelay(meas_time);				// odczekaj czas potrzebny na przetworzenie (maksymalny wymagany)

		// odczyt danych z czujników
		if(TMP117.present && TMP117.sensor_use){
		  if(TMP117.temp.use_meas) {
			  TMP117.temp.value = TMP117_get_temp();
		//    			  printf("Temperatura TMP117: %.2f\r\n", TMP117.temp.value);
		  }
		}
		if(BME280.present && BME280.sensor_use){
		  if(BME280.temp.use_meas) {
			  BME280.temp.value = BME280_get_temp();
		//    			  printf("Temperatura BME280: %.2f\r\n", BME280.temp.value);
		  }
		  if(BME280.press.use_meas) {
			  BME280.press.value = BME280_get_press();
		//    			  printf("Cisnienie BME280: %.2f\r\n", BME280.press.value);
		  }
		  if(BME280.hum.use_meas) {
			  BME280.hum.value = BME280_get_hum();
		//    		      printf("Wilgotnosc BME280: %.2f\r\n", BME280.hum.value);
		  }
		}
		if(SHT3.present && SHT3.sensor_use){
		  SHTC3_read_values(shtc3_values);
		  if(SHT3.temp.use_meas) {
			  SHT3.temp.value = SHTC3_get_temp(shtc3_values);
		//    			  printf("Temperatura SHT3: %.2f\r\n", SHT3.temp.value);
		  }
		  if(SHT3.hum.use_meas) {
			  SHT3.hum.value = SHTC3_get_hum(shtc3_values);
		//    			  printf("Wilgotnosc SHT3: %.2f\r\n", SHT3.hum.value);
		  }
		}
		if(MS8607.present && MS8607.sensor_use){
		  if(MS8607.temp.use_meas) {
			  MS8607.temp.value = MS8607_get_temp();
		//    			  printf("Temperatura MS8607: %.2f\r\n", MS8607.temp.value);
		  }
		  if(MS8607.press.use_meas) {
			  MS8607.press.value = MS8607_get_press();
		//    			  printf("Cisnienie MS8607: %.2f\r\n", MS8607.press.value);
		  }
		  if(MS8607.hum.use_meas) {
			  MS8607.hum.value = MS8607_get_hum();
		//    			  printf("Wilgotnosc MS8607: %.2f\r\n", MS8607.hum.value);
		  }
		}
		if(DPS368.present && DPS368.sensor_use){
		  float dps_scaled_temp = DPS368_get_scaled_temp();				// odczytaj temperature
		  if(DPS368.temp.use_meas) {
			  DPS368.temp.value = DPS368_calc_temp(dps_scaled_temp);
		//    			  printf("Temperatura DPS368: %.2f\r\n", DPS368.temp.value);
		  }
		  if(dps368_press) {											// jak ma byc cisnienie z DPS
			  DPS368_start_meas_press(dps368_ovr_press);				// uruchom przetworzenie cisnienia
			  osDelay( calcBusyTime(dps368_ovr_press) + 10);			// zaczekaj na koniec przetwarzania
   			  DPS368.press.value = DPS368_get_press(dps_scaled_temp);	// pobierz cisnienie uzywając temperatury
//    		  printf("Cisnienie DPS368: %.2f\r\n", DPS368.press.value);
		  }
		}
		sensors_data_ready = 1;
	}		// while(1)
}

// ******************************************************************************************************

void THP_MainTask(void const *argument)
{
	  POWER_OFF();
	  if(!Power_SW_READ()) HAL_NVIC_SystemReset();		// nie nacisniety power -> reset CPU
	  HAL_UART_RxCpltCallback(&huart1); //CLI
	  HAL_UART_RxCpltCallback(&huart2); //SIM
	  check_powerOn();
	  if(!Power_SW_READ()) HAL_NVIC_SystemReset();		// nie nacisniety power -> reset CPU
	  SIM_HW_OFF();
	  printf("\r\n\r\n\r\nInitializing (RTOS version)...\r\n");
	  if (Load_config()==0) {printf("Config loaded OK \r\n");};
	  charger_state = BQ25798_check();
	  if (charger_state) {
		  printf("Configure charger \r\n");
		  QON_EN();
		  BQ25798_Sys_Min_Voltage_write(3); 	// 3250mV
		  BQ25798_Chr_Volt_Limit_write(4200); 	// 4200mV
		  BQ25798_Chr_Curr_Limit_write(2000); 	// 2000mA
		  BQ25798_Chr_Input_Voltage_Limit_write(130); //*100mV
		  BQ25798_Chr_Input_Curr_Limit_write(200); //*10mA
		  BQ25798_Chrg_CTRL1_write(0x95);
		  BQ25798_Chrg_NTC_CTRL1_write(1);
		  CE_EN();
		  BQ25798_MPPT_CTRL(1); //MPPT ON

	  }
	  LED1_ON();
	  LED2_OFF();
	  ADC_DMA_Start();

	  TMP117.present = TMP117_check();
	  SHT3.present = SHTC3_check();
	  MS8607.present = MS8607_check();
	  BME280.present = BME280_check();
	  DPS368.present = DPS368_check();

	  getConfVars();

	  tmp117_avr=tmp117_avr_conf(TMP117.sensor_conf);
	//  printf("TMP117 conf var %x\r\n", tmp117_avr);
	  dps368_ovr_conf=dps368_ovr_config(DPS368.sensor_conf);
	  printf("DPS368 conf var %x\r\n", dps368_ovr_conf);
	  dps368_ovr_temp = (uint8_t)(dps368_ovr_conf >> 8);
	  dps368_ovr_press = (uint8_t)dps368_ovr_conf;

	  DPS368_init(FIFO_DIS, INT_NONE);
	  DPS368_temp_correct(dps368_ovr_temp);

	  sht3_mode=SHT3.sensor_conf;
	  if(sht3_mode==normal) printf("SHTC3 normal mode\r\n");
	  else printf("SHTC3 low power mode\r\n");

	  bme280_conf_change(BME280.sensor_conf);

	  MS8607_osr(MS8607.sensor_conf);
	  printf("MS8607 OSR %d\r\n", 256<<MS8607.sensor_conf);

	  disp_type = config.disp_type;
	  new_tim_interval = config.tim_interval; //w sekundach

	  // uruchomienie taska sensorów
	  osThreadDef(SensorTask, SensorsTask, osPriorityNormal, 0, 512);
	  measTaskHandle = osThreadCreate(osThread(SensorTask), NULL);

	  uint32_t ticks30ms = HAL_GetTick();
	  uint32_t ticksbqwd = HAL_GetTick();
	  uint8_t firstrun = 1;
	  osDelay(100);

	  while (1)
	  {
		  if (new_tim_interval != tim_interval) {
			  tim_interval = new_tim_interval;
			  config.tim_interval = tim_interval;
		  }

		  CLI();

		  // miganie LED i test wyłącznika
		  if(HAL_GetTick()-ticks30ms >= 30)
		  {
			  ticks30ms = HAL_GetTick();
			  LED1_TOGGLE();
			  check_powerOff();
		  }

		  // reset BQ
		  if(HAL_GetTick()-ticksbqwd >= 15000)
		  {
			  ticksbqwd = HAL_GetTick();
			  BQ25798_WD_RST();
		  }

		  // uruchomienie pomiaru
		  if(HAL_GetTick() - ticksstart >= tim_interval*1000UL || firstrun) {	// czas uruchomic pomiar ?
			  ticksstart = HAL_GetTick();
			  firstrun = 0;
			  if (meas_count > 0 || meas_cont_mode) {
				  if(meas_cont_mode == 0) {
					  meas_count--;
					  if(meas_count == 0) printf("Last measure\r\n");
				  }
				  vTaskResume(measTaskHandle);						// odblokuj taks pomiarow
			  }
		  }

		  // wyswietlenie pomiarow
	      if(sensors_data_ready) {						// taks sensorow zakonczyl dzialanie ?
	    	  sensors_data_ready = 0;
	    	  if(disp_type > 0) {
	    		  display_values(disp_type);
	    	  }
	      }

	      __WFI();
	  }
}

