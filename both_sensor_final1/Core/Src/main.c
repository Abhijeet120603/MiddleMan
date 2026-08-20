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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Adafruit_AS7341.h"
#include "LTR390.h"
#include <string.h>
#include <stdio.h>

#include "Stepper.h"
#include "pump.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    uint8_t sensor;
    uint32_t duration;
    uint32_t incubation;
    uint8_t start;
} UART_Command_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SENSOR_NONE    0
#define SENSOR_AS7341  1
#define SENSOR_LTR390  2

#define LED_ON  1
#define LED_OFF 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* Definitions for AS7341_Send */
osThreadId_t AS7341_SendHandle;
const osThreadAttr_t AS7341_Send_attributes = {
  .name = "AS7341_Send",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Send_LTR390 */
osThreadId_t Send_LTR390Handle;
const osThreadAttr_t Send_LTR390_attributes = {
  .name = "Send_LTR390",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for UartSend */
osThreadId_t UartSendHandle;
const osThreadAttr_t UartSend_attributes = {
  .name = "UartSend",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Stepper_Home */
osThreadId_t Stepper_HomeHandle;
const osThreadAttr_t Stepper_Home_attributes = {
  .name = "Stepper_Home",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for White_Led_Align */
osThreadId_t White_Led_AlignHandle;
const osThreadAttr_t White_Led_Align_attributes = {
  .name = "White_Led_Align",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for UV_Led_Align */
osThreadId_t UV_Led_AlignHandle;
const osThreadAttr_t UV_Led_Align_attributes = {
  .name = "UV_Led_Align",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Aspirate */
osThreadId_t AspirateHandle;
const osThreadAttr_t Aspirate_attributes = {
  .name = "Aspirate",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Command_Dispatc */
osThreadId_t Command_DispatcHandle;
const osThreadAttr_t Command_Dispatc_attributes = {
  .name = "Command_Dispatc",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for commandQueue */
osMessageQueueId_t commandQueueHandle;
const osMessageQueueAttr_t commandQueue_attributes = {
  .name = "commandQueue"
};
/* Definitions for messageI2C1_Lock */
osSemaphoreId_t messageI2C1_LockHandle;
const osSemaphoreAttr_t messageI2C1_Lock_attributes = {
  .name = "messageI2C1_Lock"
};
/* USER CODE BEGIN PV */
Adafruit_AS7341_t as7341;
uint16_t spectral[10];

volatile uint32_t uv340 = 0;

uint8_t rxByte;
char rxBuffer[20];
uint8_t rxIndex = 0;

//volatile uint8_t currentSensor = 0;
//
//volatile uint32_t duration = 0;

volatile UART_Command_t uartCmd = {0};

uint8_t AS7341_Present = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
void StartAS7341Task(void *argument);
void StartLTR390(void *argument);
void SendUART(void *argument);
void StartStepperHome(void *argument);
void White_Led(void *argument);
void UV_Led(void *argument);
void Aspirate_Sample(void *argument);
void Command_Dispatcher(void *argument);

/* USER CODE BEGIN PFP */
void ControlWhiteLED(uint8_t state);
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
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of messageI2C1_Lock */
  messageI2C1_LockHandle = osSemaphoreNew(1, 1, &messageI2C1_Lock_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */


  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of commandQueue */
  commandQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &commandQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of AS7341_Send */
  AS7341_SendHandle = osThreadNew(StartAS7341Task, NULL, &AS7341_Send_attributes);

  /* creation of Send_LTR390 */
  Send_LTR390Handle = osThreadNew(StartLTR390, NULL, &Send_LTR390_attributes);

  /* creation of UartSend */
  UartSendHandle = osThreadNew(SendUART, NULL, &UartSend_attributes);

  /* creation of Stepper_Home */
  Stepper_HomeHandle = osThreadNew(StartStepperHome, NULL, &Stepper_Home_attributes);

  /* creation of White_Led_Align */
  White_Led_AlignHandle = osThreadNew(White_Led, NULL, &White_Led_Align_attributes);

  /* creation of UV_Led_Align */
  UV_Led_AlignHandle = osThreadNew(UV_Led, NULL, &UV_Led_Align_attributes);

  /* creation of Aspirate */
  AspirateHandle = osThreadNew(Aspirate_Sample, NULL, &Aspirate_attributes);

  /* creation of Command_Dispatc */
  Command_DispatcHandle = osThreadNew(Command_Dispatcher, NULL, &Command_Dispatc_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  Stepper_IsGrooveDetected();
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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 799;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  HAL_UART_Receive_IT(&huart1, &rxByte, 1);
  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, STEP_Pin|DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, EN_Pin|MS1_Pin|MS2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DS18B20_Pin|White_LED_Pin|UV_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : STEP_Pin EN_Pin DIR_Pin MS1_Pin
                           MS2_Pin */
  GPIO_InitStruct.Pin = STEP_Pin|EN_Pin|DIR_Pin|MS1_Pin
                          |MS2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Groove_Sensor_In_Pin */
  GPIO_InitStruct.Pin = Groove_Sensor_In_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Groove_Sensor_In_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DS18B20_Pin */
  GPIO_InitStruct.Pin = DS18B20_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DS18B20_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : White_LED_Pin UV_LED_Pin */
  GPIO_InitStruct.Pin = White_LED_Pin|UV_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rxByte == '\r' || rxByte == '\n')
        {
            // Only process if we have data
            if (rxIndex > 0)
            {
                rxBuffer[rxIndex] = '\0';

                // Trim trailing newline if present
                if (rxBuffer[rxIndex - 1] == '\n')
                {
                    rxBuffer[rxIndex - 1] = '\0';
                }

                // Check for new commands
                if (strcmp(rxBuffer, "PING") == 0)
                {
                    HAL_UART_Transmit(&huart1,
                                      (uint8_t *)"PONG\r\n",
                                      6,
                                      HAL_MAX_DELAY);
                }

                else if (strcmp(rxBuffer, "HELP") == 0 || strcmp(rxBuffer, "help") == 0)
                {
                    const char *helpMsg =
                        "Available commands:\r\n"
                        "PING - Test connection\r\n"
                        "STEPPER_HOME - Home the stepper motor\r\n"
                        "WHITE_LED_ALIGN - Start white LED alignment\r\n"
                        "UV_LED_ALIGN - Start UV LED alignment\r\n"
                        "ASPIRATE - Run aspirate sample sequence\r\n"
                        "AS7341,<duration>,<incubation> - AS7341 sensor measurement\r\n"
                        "LTR390,<duration>,<incubation> - LTR390 sensor measurement\r\n";

                    HAL_UART_Transmit(&huart1, (uint8_t *)helpMsg, strlen(helpMsg), HAL_MAX_DELAY);
                }

                else if (strcmp(rxBuffer, "STEPPER_HOME") == 0 ||
                         strcmp(rxBuffer, "stepper_home") == 0)
                {
                    // Set command for stepper home
                    uartCmd.sensor = SENSOR_NONE;
                    uartCmd.duration = 0;
                    uartCmd.incubation = 0;
                    uartCmd.start = 2;  // Use 2 for stepper home
                }

                else if (strcmp(rxBuffer, "WHITE_LED_ALIGN") == 0 ||
                         strcmp(rxBuffer, "white_led_align") == 0)
                {
                    uartCmd.sensor = SENSOR_NONE;
                    uartCmd.duration = 0;
                    uartCmd.incubation = 0;
                    uartCmd.start = 3;  // Use 3 for white LED align
                }

                else if (strcmp(rxBuffer, "UV_LED_ALIGN") == 0 ||
                         strcmp(rxBuffer, "uv_led_align") == 0)
                {
                    uartCmd.sensor = SENSOR_NONE;
                    uartCmd.duration = 0;
                    uartCmd.incubation = 0;
                    uartCmd.start = 4;  // Use 4 for UV LED align
                }

                else if (strcmp(rxBuffer, "ASPIRATE") == 0 ||
                         strcmp(rxBuffer, "aspirate") == 0 ||
                         strcmp(rxBuffer, "ASPIRATE_SAMPLE") == 0)
                {
                    uartCmd.sensor = SENSOR_NONE;
                    uartCmd.duration = 0;
                    uartCmd.incubation = 0;
                    uartCmd.start = 5;  // Use 5 for aspirate sample
                }

                else
                {
                    // Existing sensor command parsing
                    char sensor[20];
                    int sec;
                    int incubate;

                    // Try parsing with 3 parameters: sensor,duration,incubation
                    if (sscanf(rxBuffer, "%[^,],%d,%d", sensor, &sec, &incubate) == 3)
                    {
                        if (strcmp(sensor, "AS7341") == 0)
                        {
                            uartCmd.sensor = SENSOR_AS7341;
                            uartCmd.duration = sec;
                            uartCmd.incubation = incubate;
                            uartCmd.start = 1;
                        }
                        else if (strcmp(sensor, "LTR390") == 0)
                        {
                            uartCmd.sensor = SENSOR_LTR390;
                            uartCmd.duration = sec;
                            uartCmd.incubation = incubate;
                            uartCmd.start = 1;
                        }
                    }
                    // Fallback to 2 parameters for backward compatibility
                    else if (sscanf(rxBuffer, "%[^,],%d", sensor, &sec) == 2)
                    {
                        if (strcmp(sensor, "AS7341") == 0)
                        {
                            uartCmd.sensor = SENSOR_AS7341;
                            uartCmd.duration = sec;
                            uartCmd.incubation = 0;  // No incubation
                            uartCmd.start = 1;
                        }
                        else if (strcmp(sensor, "LTR390") == 0)
                        {
                            uartCmd.sensor = SENSOR_LTR390;
                            uartCmd.duration = sec;
                            uartCmd.incubation = 0;  // No incubation
                            uartCmd.start = 1;
                        }
                    }
                }

                // Reset buffer for next command
                rxIndex = 0;
                memset(rxBuffer, 0, sizeof(rxBuffer));
            }
        }
        else if (rxByte != '\r' && rxByte != '\n')
        {
            if (rxIndex < sizeof(rxBuffer) - 1)
            {
                rxBuffer[rxIndex++] = rxByte;
            }
            else
            {
                // Buffer overflow - reset
                rxIndex = 0;
                memset(rxBuffer, 0, sizeof(rxBuffer));
            }
        }

        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}

void ControlWhiteLED(uint8_t state)
{
    if (state == LED_ON)
    {
        HAL_GPIO_WritePin(White_LED_GPIO_Port, White_LED_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(White_LED_GPIO_Port, White_LED_Pin, GPIO_PIN_RESET);
    }
}


//void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
//{
//    Stepper_TimerPulseFinishedCallback(htim);
//}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartAS7341Task */
/**
  * @brief  Function implementing the AS7341_Send thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartAS7341Task */
void StartAS7341Task(void *argument)
{
  /* USER CODE BEGIN 5 */
  for(;;)
  {
      if(uartCmd.start && uartCmd.sensor == SENSOR_AS7341)
      {

    	  HAL_I2C_DeInit(&hi2c1);
    	  HAL_Delay(5);
    	  HAL_I2C_Init(&hi2c1);

    	  if (HAL_I2C_IsDeviceReady(&hi2c1,
    	                            AS7341_I2CADDR_DEFAULT << 1,
    	                            5,
    	                            100) != HAL_OK)
    	  {
    	      HAL_UART_Transmit(&huart1,
    	                        (uint8_t *)"AS7341 NOT FOUND\r\n",
    	                        19,
    	                        HAL_MAX_DELAY);

    	      uartCmd.start = 0;
    	      uartCmd.sensor = SENSOR_NONE;
    	      continue;
    	  }

    	  Adafruit_AS7341_Init(&as7341);

    	  if (!Adafruit_AS7341_begin(&as7341,
    	                             AS7341_I2CADDR_DEFAULT,
    	                             &hi2c1,
    	                             0))
    	  {
    	      HAL_UART_Transmit(&huart1,
    	                        (uint8_t *)"AS7341 INIT FAILED\r\n",
    	                        20,
    	                        HAL_MAX_DELAY);

    	      uartCmd.start = 0;
    	      uartCmd.sensor = SENSOR_NONE;
    	      continue;
    	  }

    	  /* Sensor is connected - initialize it again */
    	  Adafruit_AS7341_Init(&as7341);

    	  if (!Adafruit_AS7341_begin(&as7341,
    	                             AS7341_I2CADDR_DEFAULT,
    	                             &hi2c1,
    	                             0))
    	  {
    	      HAL_UART_Transmit(&huart1,
    	                        (uint8_t *)"AS7341 INIT FAILED\r\n",
    	                        20,
    	                        HAL_MAX_DELAY);

    	      uartCmd.start = 0;
    	      uartCmd.sensor = SENSOR_NONE;
    	      continue;
    	  }

    	  Adafruit_AS7341_setATIME(&as7341, 50);
    	  Adafruit_AS7341_setASTEP(&as7341, 999);
    	  Adafruit_AS7341_setGain(&as7341, AS7341_GAIN_16X);
          // Ensure LED is off initially
          ControlWhiteLED(LED_OFF);

          // Incubation period handling with LED control
          if (uartCmd.incubation > 0)
          {
              // Turn on LED 1 second before incubation ends
              if (uartCmd.incubation > 1)
              {
                  // Wait until 1 second before incubation ends
                  uint32_t ledOnTick = osKernelGetTickCount() + ((uartCmd.incubation - 1) * 1000);
                  osDelayUntil(ledOnTick);

                  // Turn on White LED
                  ControlWhiteLED(LED_ON);

                  // Wait for the remaining 1 second
                  uint32_t remainingTick = osKernelGetTickCount() + 1000;
                  osDelayUntil(remainingTick);
              }
              else
              {
                  // Incubation is 1 second or less - turn on LED immediately
                  ControlWhiteLED(LED_ON);

                  // Wait for incubation time
                  uint32_t incubateTick = osKernelGetTickCount() + (uartCmd.incubation * 1000);
                  osDelayUntil(incubateTick);
              }
          }
          else
          {
              // No incubation - turn on LED immediately
              ControlWhiteLED(LED_ON);
          }

          // Start measurements
          uint32_t tick = osKernelGetTickCount();

          for (uint32_t i = 0; i < uartCmd.duration; i++)
          {
              if(Adafruit_AS7341_take10ChannelReadings(&as7341, spectral))
              {
                  char tx[100];
                  snprintf(tx,
                           sizeof(tx),
                           "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
                           i + 1,
                           spectral[0], spectral[1], spectral[2], spectral[3],
                           spectral[4], spectral[5], spectral[6], spectral[7],
                           spectral[8], spectral[9]);

                  HAL_UART_Transmit(&huart1,
                                    (uint8_t *)tx,
                                    strlen(tx),
                                    HAL_MAX_DELAY);
              }

              tick += 1000;
              osDelayUntil(tick);
          }

          // Turn off LED after measurements complete
          ControlWhiteLED(LED_OFF);

          uartCmd.start = 0;
          uartCmd.sensor = SENSOR_NONE;
      }

      osDelay(10);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartLTR390 */
/**
* @brief Function implementing the Send_LTR390 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLTR390 */
void StartLTR390(void *argument)
{
  /* USER CODE BEGIN StartLTR390 */
  LTR390_HandleTypeDef ltr;

  LTR390_Init(&ltr, &hi2c3);

  if(!LTR390_Begin(&ltr))
  {
	  osDelay(1);
//      Error_Handler();

  }

//  LTR390_SetMode(&ltr, LTR390_MODE_UVS);
//  LTR390_SetGain(&ltr, LTR390_GAIN_18);
//  LTR390_SetResolution(&ltr, LTR390_RESOLUTION_20BIT);

  for(;;)
  {
      if(uartCmd.start && uartCmd.sensor == SENSOR_LTR390)
      {
    	  if (HAL_I2C_IsDeviceReady(&hi2c3,
    	                            0X53 << 1,
    	                            2,
    	                            100) != HAL_OK)
    	  {
    	      HAL_UART_Transmit(&huart1,
    	                        (uint8_t *)"LTR390 NOT FOUND\r\n",
    	                        19,
    	                        HAL_MAX_DELAY);

    	      uartCmd.start = 0;
    	      uartCmd.sensor = SENSOR_NONE;
    	      continue;
    	  }

    	  /* Reinitialize every time */
    	  LTR390_Init(&ltr, &hi2c3);

    	  if (!LTR390_Begin(&ltr))
    	  {
    	      HAL_UART_Transmit(&huart1,
    	                        (uint8_t *)"LTR390 INIT FAILED\r\n",
    	                        20,
    	                        HAL_MAX_DELAY);

    	      uartCmd.start = 0;
    	      uartCmd.sensor = SENSOR_NONE;
    	      continue;
    	  }

    	  LTR390_SetMode(&ltr, LTR390_MODE_UVS);
    	  LTR390_SetGain(&ltr, LTR390_GAIN_18);
    	  LTR390_SetResolution(&ltr, LTR390_RESOLUTION_20BIT);
          // Ensure UV LED is off initially
          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

          // Incubation period handling with LED control
          if (uartCmd.incubation > 0)
          {
              // Turn on UV LED 1 second before incubation ends
              if (uartCmd.incubation > 1)
              {
                  // Wait until 1 second before incubation ends
                  uint32_t ledOnTick = osKernelGetTickCount() + ((uartCmd.incubation - 1) * 1000);
                  osDelayUntil(ledOnTick);

                  // Turn on UV LED
                  HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);

                  // Wait for the remaining 1 second
                  uint32_t remainingTick = osKernelGetTickCount() + 1000;
                  osDelayUntil(remainingTick);
              }
              else
              {
                  // Incubation is 1 second or less - turn on UV LED immediately
                  HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);

                  // Wait for incubation time
                  uint32_t incubateTick = osKernelGetTickCount() + (uartCmd.incubation * 1000);
                  osDelayUntil(incubateTick);
              }
          }
          else
          {
              // No incubation - turn on UV LED immediately
              HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);
          }

          // Start measurements
          uint32_t tick = osKernelGetTickCount();

          for (uint32_t i = 0; i < uartCmd.duration; i++)
          {
              if (LTR390_NewDataAvailable(&ltr))
              {
                  uv340 = LTR390_ReadUVS(&ltr);

                  char tx[30];
                  snprintf(tx,
                           sizeof(tx),
                           "%lu,%lu\r\n",
                           i + 1,
                           uv340);

                  HAL_UART_Transmit(&huart1,
                                    (uint8_t *)tx,
                                    strlen(tx),
                                    HAL_MAX_DELAY);
              }

              tick += 1000;
              osDelayUntil(tick);
          }

          // Turn off UV LED after measurements complete
          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

          uartCmd.start = 0;
          uartCmd.sensor = SENSOR_NONE;
      }

      osDelay(10);
  }
  /* USER CODE END StartLTR390 */
}

/* USER CODE BEGIN Header_SendUART */
/**
* @brief Function implementing the UartSend thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SendUART */
void SendUART(void *argument)
{
  /* USER CODE BEGIN SendUART */
//    const char pong[] = "PONG\r\n";

    for (;;)
    {
        osDelay(1000);
    }
  /* USER CODE END SendUART */
}

/* USER CODE BEGIN Header_StartStepperHome */
/**
* @brief Function implementing the Stepper_Home thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartStepperHome */
void StartStepperHome(void *argument)
{
  /* USER CODE BEGIN StartStepperHome */
//	Stepper_Home();
  /* Infinite loop */
    for(;;)
    {
        if (uartCmd.start == 2)  // Stepper home command
        {
            // Send acknowledgment
            HAL_UART_Transmit(&huart1, (uint8_t *)"STEPPER HOME START\r\n", 21, HAL_MAX_DELAY);

            // Call your stepper home function
            Stepper_Home();


            HAL_UART_Transmit(&huart1, (uint8_t *)"STEPPER HOME COMPLETE\r\n", 24, HAL_MAX_DELAY);

            // Reset command
            uartCmd.start = 0;
        }

        osDelay(10);
    }
  /* USER CODE END StartStepperHome */
}

/* USER CODE BEGIN Header_White_Led */
/**
* @brief Function implementing the White_Led_Align thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_White_Led */
void White_Led(void *argument)
{
  /* USER CODE BEGIN White_Led */
  /* Infinite loop */
    for(;;)
    {
        if (uartCmd.start == 3)  // White LED align command
        {
            // Send acknowledgment
            HAL_UART_Transmit(&huart1, (uint8_t *)"WHITE LED ALIGN START\r\n", 24, HAL_MAX_DELAY);

            // Add your white LED alignment logic here
            // For example, you might:
            // - Turn on white LED
            Stepper_White_LED_Align();


            HAL_UART_Transmit(&huart1, (uint8_t *)"WHITE LED ALIGN COMPLETE\r\n", 27, HAL_MAX_DELAY);

            // Reset command
            uartCmd.start = 0;
        }

        osDelay(10);
    }
  /* USER CODE END White_Led */
}

/* USER CODE BEGIN Header_UV_Led */
/**
* @brief Function implementing the UV_Led_Align thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_UV_Led */
void UV_Led(void *argument)
{
  /* USER CODE BEGIN UV_Led */
//	Stepper_UV_Sensor_Align();
  /* Infinite loop */
    for(;;)
    {
        if (uartCmd.start == 4)  // UV LED align command
        {
            // Send acknowledgment
            HAL_UART_Transmit(&huart1, (uint8_t *)"UV LED ALIGN START\r\n", 21, HAL_MAX_DELAY);

            Stepper_UV_Sensor_Align();

            HAL_UART_Transmit(&huart1, (uint8_t *)"UV LED ALIGN COMPLETE\r\n", 24, HAL_MAX_DELAY);

            // Reset command
            uartCmd.start = 0;
        }

        osDelay(10);
    }
  /* USER CODE END UV_Led */
}

/* USER CODE BEGIN Header_Aspirate_Sample */
/**
* @brief Function implementing the Aspirate thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Aspirate_Sample */
void Aspirate_Sample(void *argument)
{
  /* USER CODE BEGIN Aspirate_Sample */
    // Initialize the pump with TIM3 handle
    PUMP_Init(&htim3);

    for(;;)
    {
        if (uartCmd.start == 5)  // Aspirate sample command
        {
            // Send acknowledgment
            HAL_UART_Transmit(&huart1,
                              (uint8_t *)"ASPIRATE SAMPLE START\r\n",
                              24,
                              HAL_MAX_DELAY);

            // Perform aspirate sequence
            // Forward (aspirate) - 10 seconds at 50% speed
            PUMP_Move(PUMP_FORWARD, 10, 50);

            // Small delay between directions
            osDelay(100);

            // Reverse (dispense) - 5 seconds at 75% speed
            PUMP_Move(PUMP_REVERSE, 5, 75);

            // Ensure pump is stopped
            PUMP_Stop();

            // Send completion message
            HAL_UART_Transmit(&huart1,
                              (uint8_t *)"ASPIRATE SAMPLE COMPLETE\r\n",
                              27,
                              HAL_MAX_DELAY);

            // Reset command
            uartCmd.start = 0;
        }

        osDelay(10);
    }
  /* USER CODE END Aspirate_Sample */
}

/* USER CODE BEGIN Header_Command_Dispatcher */
/**
* @brief Function implementing the Command_Dispatc thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Command_Dispatcher */
void Command_Dispatcher(void *argument)
{
  /* USER CODE BEGIN Command_Dispatcher */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Command_Dispatcher */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
