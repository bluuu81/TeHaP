/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
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

uint16_t tmp117_avr;
volatile uint8_t dps368_ovr_temp;
volatile uint8_t dps368_ovr_press;
volatile uint16_t dps368_ovr_conf;
uint8_t sht3_mode;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_I2C3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CRC_Init(void);
static void MX_TIM16_Init(void);
void MX_TIM6_Init(uint16_t tim_int);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();
  MX_TIM16_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);	// LED2 na PWM
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_UART_RxCpltCallback(&huart1); //CLI
  HAL_UART_RxCpltCallback(&huart2); //SIM
  check_powerOn();
  SIM_HW_OFF();
  printf("\r\n\r\n\r\nInitializing ...\r\n");
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
  switch (MS8607.sensor_conf)
  {
  	  case 0:
  		printf("MS8607 OSR 256\r\n");
  		break;
  	  case 1:
  		printf("MS8607 OSR 512\r\n");
  		break;
  	  case 2:
  		printf("MS8607 OSR 1024\r\n");
  		break;
  	  case 3:
  		printf("MS8607 OSR 2048\r\n");
  		break;
  	  case 4:
  		printf("MS8607 OSR 4096\r\n");
  		break;
  	  case 5:
  		printf("MS8607 OSR 8192\r\n");
  		break;
  	  default:
  		break;
  }

  disp_type = config.disp_type;

  new_tim_interval = config.tim_interval; //w sekundach

  MX_TIM6_Init(tim_interval);
  uint8_t meas2disp = 0;
  uint8_t meas2disp_dps = 0;

  uint32_t ticks30ms = HAL_GetTick();
  uint32_t ticks_meas = HAL_GetTick();
  uint32_t dps_ticks_meas = HAL_GetTick();
  uint32_t ticksbqwd = HAL_GetTick();
  uint16_t meas_time = 100;
  uint16_t dps_meas_time = 200;

  uint8_t dps368_press = 0;
  uint8_t dps368_press_ready = 0;
  float dps_scaled_temp;
  uint8_t shtc3_values[6];

//  LED2_ON();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (new_tim_interval != tim_interval) {
		  tim_interval = new_tim_interval;
		  config.tim_interval = tim_interval;
	      ReinitTimer(tim_interval);
	  }

	  thp_loop();

	  if(HAL_GetTick()-ticks30ms >= 30)
	  {
		  ticks30ms = HAL_GetTick();
		  LED1_TOGGLE();
		  check_powerOff();
	  }

	  if(HAL_GetTick()-ticksbqwd >= 15000)
	  {
		  ticksbqwd = HAL_GetTick();
		  BQ25798_WD_RST();
	  }


	  if(meas_start) {
		  if (meas_count > 0 || meas_cont_mode) {
			  if(meas_cont_mode == 0) meas_count--;
			  if(TMP117.present){
				  if(TMP117.sensor_use && TMP117.temp.use_meas) {
					  TMP117_start_meas(tmp117_avr);
					  meas_time += 200;
				  }
			  }
			  if(BME280.present){
				  if(BME280.sensor_use && (BME280.temp.use_meas || BME280.press.use_meas || BME280.hum.use_meas) ) {
					  BME280_start_meas();
					  meas_time += 500;
				  }
			  }
			  if(SHT3.present){
				  if(SHT3.sensor_use && (SHT3.temp.use_meas || SHT3.hum.use_meas)) {
					  SHTC3_start_meas(sht3_mode);
					  meas_time += 100;
				  }
			  }
			  if(DPS368.present){
				  if(DPS368.sensor_use && (DPS368.temp.use_meas || DPS368.press.use_meas)) {
					  DPS368_start_meas_temp(dps368_ovr_temp);
					  meas_time += calcBusyTime(dps368_ovr_temp);
					  if (DPS368.press.use_meas) {
						  dps368_press = 1;
					  }
				  }
			  }
			  meas_ready = 1;
			  ticks_meas = HAL_GetTick();
			  dps_ticks_meas = HAL_GetTick();
			  meas_start = 0;
			  if(disp_type == 1) {
				  printf("Komenda startu pomiarow wyslana\r\n");
				  printf("Meas interval: %u\r\n", tim_interval);
			  }
			  if (meas_count == 0 && meas_cont_mode == 0) {
				  HAL_TIM_Base_Stop_IT(&htim6);
			  }
		  }
	  }



      if ((meas_ready) && ((HAL_GetTick() - ticks_meas) >= meas_time)) {
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
    		  dps_scaled_temp = DPS368_get_scaled_temp();
    		  if(DPS368.temp.use_meas) {
    			  DPS368.temp.value = DPS368_calc_temp(dps_scaled_temp);
//    			  printf("Temperatura DPS368: %.2f\r\n", DPS368.temp.value);
    		  }
    	  }
          meas_ready = 0;
	      meas_time = 200;
	      meas2disp = 1;


		  if(dps368_press) {
			  DPS368_start_meas_press(dps368_ovr_press);
			  dps_meas_time = calcBusyTime(dps368_ovr_press);
			  dps368_press = 0;
			  dps368_press_ready = 1;
		  }
      }

      if (dps368_press_ready && ((HAL_GetTick() - dps_ticks_meas) >= dps_meas_time)) {
   		  if(DPS368.sensor_use && DPS368.press.use_meas) {
   			  DPS368.press.value = DPS368_get_press(dps_scaled_temp);
//    		  printf("Cisnienie DPS368: %.2f\r\n", DPS368.press.value);
    		  dps368_press_ready = 0;
    		  meas2disp_dps = 1;
   		  }
      }

      if(disp_type > 0) {
    	  if(DPS368.sensor_use && DPS368.press.use_meas) {
    		  if(meas2disp_dps) {
    			  display_values(disp_type);
    			  meas2disp_dps = 0;
    		  }
    	  } else if(meas2disp) {
			  display_values(disp_type);
			  meas2disp = 0;
    	  }
      }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* USART1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USART2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = ENABLE;
  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_16;
  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_NONE;
  hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
  hcrc.Init.GeneratingPolynomial = 0x31;
  hcrc.Init.CRCLength = CRC_POLYLENGTH_8B;
  hcrc.Init.InitValue = 0xFF;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00301347;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00301347;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00301347;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM6_Init(uint16_t tim_int)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = (SystemCoreClock / 1000) - 1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = (tim_int * 1000) - 1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 312;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 256;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */
  HAL_TIM_MspPostInit(&htim16);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SIM_UART_DTR_Pin|SIM_GPS_Pin|RST2_Pin|BQ_QON_Pin
                          |BQ_CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SIM_PWR_Pin|RST3_Pin|LED1_Pin|Main_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : But_ONOFF_Pin */
  GPIO_InitStruct.Pin = But_ONOFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(But_ONOFF_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_UART_RI_Pin */
  GPIO_InitStruct.Pin = SIM_UART_RI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SIM_UART_RI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SIM_UART_DTR_Pin SIM_GPS_Pin BQ_QON_Pin BQ_CE_Pin */
  GPIO_InitStruct.Pin = SIM_UART_DTR_Pin|SIM_GPS_Pin|BQ_QON_Pin|BQ_CE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SIM_PWR_Pin LED1_Pin Main_SW_Pin */
  GPIO_InitStruct.Pin = SIM_PWR_Pin|LED1_Pin|Main_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SIM_WDT_Pin BQ_INT_Pin */
  GPIO_InitStruct.Pin = SIM_WDT_Pin|BQ_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : TP7_Pin TP8_Pin PB11 PB3 */
  GPIO_InitStruct.Pin = TP7_Pin|TP8_Pin|GPIO_PIN_11|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RST3_Pin */
  GPIO_InitStruct.Pin = RST3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RST3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RST2_Pin */
  GPIO_InitStruct.Pin = RST2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RST2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PH3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM6) {
        // Zmienna start ustawiona na 1 co określony okres czasu
        meas_start = 1;
    }
}

void ReinitTimer(uint16_t period) {
    HAL_TIM_Base_DeInit(&htim6);
    MX_TIM6_Init(period);
    HAL_TIM_Base_Start_IT(&htim6);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  printf("Hard fault ...\r\n");
  LED1_OFF();
  LED2_ON();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
