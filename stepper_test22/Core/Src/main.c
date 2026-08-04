/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  Stepper_Disable();

  HAL_Delay(100);

//  Set_Microstepping(MICROSTEP_SIXTEENTH);  // Most precise
//  HAL_Delay(10);
  // Enable driver (LOW = Enable for A4988/DRV8825)
//  HAL_GPIO_WritePin(En_GPIO_Port, En_Pin, GPIO_PIN_RESET);

  // Set microstepping (Full Step)
//  HAL_GPIO_WritePin(MS1_GPIO_Port, MS1_Pin, GPIO_PIN_RESET);
//  HAL_GPIO_WritePin(MS2_GPIO_Port, MS2_Pin, GPIO_PIN_RESET);

  // Start STEP pulses
//  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

//  HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
//  HAL_Delay(1000);
//
//  HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // Enable motor before moving
      // Enable motor before moving
      Stepper_Enable();
      HAL_Delay(10);  // Small delay for driver to settle

      // Move forward (clockwise) for 5 seconds at 1000 Hz
      Move_Stepper(POSITIVE, 5000, 1000);

//      Stepper_Disable();

      // Rest for 3 seconds
      HAL_Delay(3000);

//      // Move backward (counter-clockwise) for 5 seconds at 1000 Hz
//      Move_Stepper(NEGATIVE, 5000, 1000);
//
//      // Disable motor after movement to save power
//      Stepper_Disable();
//
//      // Rest for 3 seconds before next cycle
//      HAL_Delay(3000);


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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, En_Pin|Dir_Pin|MS1_Pin|MS2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : En_Pin Dir_Pin MS1_Pin MS2_Pin */
  GPIO_InitStruct.Pin = En_Pin|Dir_Pin|MS1_Pin|MS2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_GPIO_WritePin(GPIOA, MS1_Pin|MS2_Pin, GPIO_PIN_SET);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void Move_Stepper(StepperDirection_t dir,
                  uint32_t duration_ms,
                  uint32_t step_frequency)
{
    uint32_t timer_clk = 16000000;      // HSI = 16 MHz

    uint32_t prescaler = 15;            // Timer clock = 1 MHz
    uint32_t timer_freq = timer_clk / (prescaler + 1);

    uint32_t arr = (timer_freq / step_frequency) - 1;

    if(arr > 0xFFFF)
        arr = 0xFFFF;

    __HAL_TIM_SET_PRESCALER(&htim2, prescaler);

    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          (arr + 1)/2);     // 50% duty

    HAL_TIM_GenerateEvent(&htim2, TIM_EVENTSOURCE_UPDATE);

    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(Dir_GPIO_Port,
                      Dir_Pin,
                      (GPIO_PinState)dir);

    HAL_Delay(1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    HAL_Delay(duration_ms);

    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
}

void Stepper_Enable(void)
{
    // EN is Active LOW on A4988/DRV8825/TMC2208
    HAL_GPIO_WritePin(En_GPIO_Port, En_Pin, GPIO_PIN_RESET);
}

void Stepper_Disable(void)
{
    // EN is Inactive HIGH
    HAL_GPIO_WritePin(En_GPIO_Port, En_Pin, GPIO_PIN_SET);
}

void Set_Microstepping(MicrostepMode_t mode)
{
    // MS1 and MS2 control microstepping on most drivers
    // MS2 | MS1 | Microstepping
    //  L  |  L  | Full step
    //  L  |  H  | Half step
    //  H  |  L  | Quarter step (A4988) / 1/8 (DRV8825)
    //  H  |  H  | 1/16 (A4988) / 1/32 (DRV8825)

    GPIO_PinState ms1_state;
    GPIO_PinState ms2_state;

    switch(mode) {
        case MICROSTEP_FULL:
            ms1_state = GPIO_PIN_RESET;
            ms2_state = GPIO_PIN_RESET;
            break;
        case MICROSTEP_HALF:
            ms1_state = GPIO_PIN_SET;
            ms2_state = GPIO_PIN_RESET;
            break;
        case MICROSTEP_QUARTER:
            ms1_state = GPIO_PIN_RESET;
            ms2_state = GPIO_PIN_SET;
            break;
        case MICROSTEP_EIGHTH:
        case MICROSTEP_SIXTEENTH:
        default:
            ms1_state = GPIO_PIN_SET;
            ms2_state = GPIO_PIN_SET;
            break;
    }

    HAL_GPIO_WritePin(MS1_GPIO_Port, MS1_Pin, ms1_state);
    HAL_GPIO_WritePin(MS2_GPIO_Port, MS2_Pin, ms2_state);
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
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
