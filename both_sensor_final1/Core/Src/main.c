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

typedef enum {
    STATE_IDLE = 0,
    STATE_WHITE_LED_ALIGN,
    STATE_UV_LED_ALIGN,
    STATE_STEPPER_HOME,
    STATE_ASPIRATE_SAMPLE,
    STATE_AS7341_MEASURE,
    STATE_LTR390_MEASURE,
	STATE_FLOW_CELL_CLEAN
} SystemState_t;
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
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for Send_LTR390 */
osThreadId_t Send_LTR390Handle;
const osThreadAttr_t Send_LTR390_attributes = {
  .name = "Send_LTR390",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh2,
};
/* Definitions for UartSend */
osThreadId_t UartSendHandle;
const osThreadAttr_t UartSend_attributes = {
  .name = "UartSend",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime1,
};
/* Definitions for Stepper_Home */
osThreadId_t Stepper_HomeHandle;
const osThreadAttr_t Stepper_Home_attributes = {
  .name = "Stepper_Home",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow5,
};
/* Definitions for White_Led_Align */
osThreadId_t White_Led_AlignHandle;
const osThreadAttr_t White_Led_Align_attributes = {
  .name = "White_Led_Align",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};
/* Definitions for UV_Led_Align */
osThreadId_t UV_Led_AlignHandle;
const osThreadAttr_t UV_Led_Align_attributes = {
  .name = "UV_Led_Align",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};
/* Definitions for Aspirate */
osThreadId_t AspirateHandle;
const osThreadAttr_t Aspirate_attributes = {
  .name = "Aspirate",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for Command_Dispatc */
osThreadId_t Command_DispatcHandle;
const osThreadAttr_t Command_Dispatc_attributes = {
  .name = "Command_Dispatc",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for Limit_sw */
osThreadId_t Limit_swHandle;
const osThreadAttr_t Limit_sw_attributes = {
  .name = "Limit_sw",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Clean_Flow_Cell */
osThreadId_t Clean_Flow_CellHandle;
const osThreadAttr_t Clean_Flow_Cell_attributes = {
  .name = "Clean_Flow_Cell",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for initialize_pump */
osThreadId_t initialize_pumpHandle;
const osThreadAttr_t initialize_pump_attributes = {
  .name = "initialize_pump",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for commandQueue */
osMessageQueueId_t commandQueueHandle;
const osMessageQueueAttr_t commandQueue_attributes = {
  .name = "commandQueue"
};
/* Definitions for pumpMutex */
osMutexId_t pumpMutexHandle;
const osMutexAttr_t pumpMutex_attributes = {
  .name = "pumpMutex"
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

volatile UART_Command_t uartCmd = {0};

uint8_t AS7341_Present = 0;

volatile SystemState_t systemState = STATE_IDLE;
volatile uint8_t limitSwitchPressed = 0;

volatile uint8_t initializePumpRequested = 0;

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
void Limit_sw_pressed(void *argument);
void Flow_Cell_Clean(void *argument);
void pump_initialize(void *argument);

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
  PUMP_Init(&htim3);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of pumpMutex */
  pumpMutexHandle = osMutexNew(&pumpMutex_attributes);

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

  /* creation of Limit_sw */
  Limit_swHandle = osThreadNew(Limit_sw_pressed, NULL, &Limit_sw_attributes);

  /* creation of Clean_Flow_Cell */
  Clean_Flow_CellHandle = osThreadNew(Flow_Cell_Clean, NULL, &Clean_Flow_Cell_attributes);

  /* creation of initialize_pump */
  initialize_pumpHandle = osThreadNew(pump_initialize, NULL, &initialize_pump_attributes);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Servo_GPIO_Port, Servo_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, STEP_Pin|DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, EN_Pin|MS1_Pin|MS2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Peltier2_Pin|Buzzer_Pin|Peltier1_Pin|DS18B20_Pin
                          |White_LED_Pin|UV_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Servo_Pin */
  GPIO_InitStruct.Pin = Servo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Servo_GPIO_Port, &GPIO_InitStruct);

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

  /*Configure GPIO pin : Limit_SW_Pin */
  GPIO_InitStruct.Pin = Limit_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Limit_SW_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Peltier2_Pin Buzzer_Pin Peltier1_Pin White_LED_Pin
                           UV_LED_Pin */
  GPIO_InitStruct.Pin = Peltier2_Pin|Buzzer_Pin|Peltier1_Pin|White_LED_Pin
                          |UV_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DS18B20_Pin */
  GPIO_InitStruct.Pin = DS18B20_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DS18B20_GPIO_Port, &GPIO_InitStruct);

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

                    /* Request startup pump initialization */
                    initializePumpRequested = 1;
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
                        "LTR390,<duration>,<incubation> - LTR390 sensor measurement\r\n"
                    	"LIMIT_SWITCH - Automatically triggers aspiration when IDLE\r\n"
                    	"CLEAN - Run flow cell cleaning\r\n";

                    HAL_UART_Transmit(&huart1, (uint8_t *)helpMsg, strlen(helpMsg), HAL_MAX_DELAY);
                }

                else if (strcmp(rxBuffer, "STEPPER_HOME") == 0 ||
                         strcmp(rxBuffer, "stepper_home") == 0)
                {
//                    // Cancel waiting state if active
//                    if (systemState == STATE_WAITING_FOR_LIMIT)
//                    {
//                        systemState = STATE_IDLE;
//                        HAL_UART_Transmit(&huart1,
//                            (uint8_t *)"WAITING CANCELLED - NEW COMMAND RECEIVED\r\n",
//                            43, HAL_MAX_DELAY);
//                    }
                    // Set command for stepper home
                    uartCmd.sensor = SENSOR_NONE;
                    uartCmd.duration = 0;
                    uartCmd.incubation = 0;
                    uartCmd.start = 2;  // Use 2 for stepper home
                }

                else if (strcmp(rxBuffer, "WHITE_LED_ALIGN") == 0 ||
                         strcmp(rxBuffer, "white_led_align") == 0)
                {
//                    // Cancel waiting state if active
//                    if (systemState == STATE_WAITING_FOR_LIMIT)
//                    {
//                        systemState = STATE_IDLE;
//                        HAL_UART_Transmit(&huart1,
//                            (uint8_t *)"WAITING CANCELLED - NEW COMMAND RECEIVED\r\n",
//                            43, HAL_MAX_DELAY);
//                    }
                    uartCmd.sensor = SENSOR_NONE;
                    uartCmd.duration = 0;
                    uartCmd.incubation = 0;
                    uartCmd.start = 3;  // Use 3 for white LED align
                }

                else if (strcmp(rxBuffer, "UV_LED_ALIGN") == 0 ||
                         strcmp(rxBuffer, "uv_led_align") == 0)
                {
//                    // Cancel waiting state if active
//                    if (systemState == STATE_WAITING_FOR_LIMIT)
//                    {
//                        systemState = STATE_IDLE;
//                        HAL_UART_Transmit(&huart1,
//                            (uint8_t *)"WAITING CANCELLED - NEW COMMAND RECEIVED\r\n",
//                            43, HAL_MAX_DELAY);
//                    }
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

                else if (strcmp(rxBuffer, "CLEAN") == 0 ||
                         strcmp(rxBuffer, "clean") == 0)
                {
                    /*
                     * Only accept CLEAN when system is IDLE.
                     */
                    if (systemState == STATE_IDLE)
                    {
                        uartCmd.sensor = SENSOR_NONE;
                        uartCmd.duration = 0;
                        uartCmd.incubation = 0;

                        /*
                         * Use 6 for CLEAN command
                         */
                        uartCmd.start = 6;

                        HAL_UART_Transmit(
                            &huart1,
                            (uint8_t *)"CLEAN COMMAND RECEIVED\r\n",
                            24,
                            HAL_MAX_DELAY
                        );
                    }
                    else
                    {
                        HAL_UART_Transmit(
                            &huart1,
                            (uint8_t *)"CANNOT CLEAN - BUSY\r\n",
                            22,
                            HAL_MAX_DELAY
                        );
                    }
                }

                else
                {
                    // Existing sensor command parsing
                    char sensor[20];
                    int sec;
                    int incubate;

//                    // Cancel waiting state if active before processing sensor commands
//                    if (systemState == STATE_WAITING_FOR_LIMIT)
//                    {
//                        systemState = STATE_IDLE;
//                        HAL_UART_Transmit(&huart1,
//                            (uint8_t *)"WAITING CANCELLED - NEW COMMAND RECEIVED\r\n",
//                            43, HAL_MAX_DELAY);
//                    }

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

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)"\r\n",
                                  2,
                                  HAL_MAX_DELAY);
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
      if(systemState == STATE_AS7341_MEASURE)
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

              systemState = STATE_IDLE;
              osThreadSetPriority(AS7341_SendHandle, osPriorityHigh1);
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

              systemState = STATE_IDLE;
              osThreadSetPriority(AS7341_SendHandle, osPriorityHigh1);
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

              systemState = STATE_IDLE;
              osThreadSetPriority(AS7341_SendHandle, osPriorityHigh1);
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

          // Reset command and return to IDLE
          uartCmd.start = 0;
          uartCmd.sensor = SENSOR_NONE;
          systemState = STATE_IDLE;
          osThreadSetPriority(AS7341_SendHandle, osPriorityHigh1);
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

  }


  for(;;)
  {
      if(systemState == STATE_LTR390_MEASURE)
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

              systemState = STATE_IDLE;
              osThreadSetPriority(Send_LTR390Handle, osPriorityHigh2);
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

              systemState = STATE_IDLE;
              osThreadSetPriority(Send_LTR390Handle, osPriorityHigh2);
              continue;
          }

          LTR390_SetMode(&ltr, LTR390_MODE_UVS);
          LTR390_SetGain(&ltr, LTR390_GAIN_18);
          LTR390_SetResolution(&ltr, LTR390_RESOLUTION_20BIT);

          // Wait for first 20-bit UV conversion
          osDelay(450);

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

          // Reset command and return to IDLE
          uartCmd.start = 0;
          uartCmd.sensor = SENSOR_NONE;
          systemState = STATE_IDLE;
          osThreadSetPriority(Send_LTR390Handle, osPriorityHigh2);
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

    for (;;)
    {
        osDelay(10);
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
        if (systemState == STATE_STEPPER_HOME)  // Stepper home command
        {
            // Send acknowledgment
            HAL_UART_Transmit(&huart1, (uint8_t *)"STEPPER HOME START\r\n", 21, HAL_MAX_DELAY);

            // Call your stepper home function
            Stepper_Home();


            HAL_UART_Transmit(&huart1, (uint8_t *)"STEPPER HOME COMPLETE\r\n", 24, HAL_MAX_DELAY);

            // Return to IDLE
            systemState = STATE_IDLE;
            osThreadSetPriority(Stepper_HomeHandle, osPriorityLow5);
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
      if (systemState == STATE_WHITE_LED_ALIGN)
      {
        HAL_UART_Transmit(&huart1, (uint8_t *)"WHITE LED ALIGN START\r\n", 24, HAL_MAX_DELAY);

        // Perform alignment

        Stepper_White_LED_Align();


        HAL_UART_Transmit(&huart1, (uint8_t *)"WHITE LED ALIGN COMPLETE\r\n", 27, HAL_MAX_DELAY);

        // Return to IDLE
        systemState = STATE_IDLE;
        osThreadSetPriority(White_Led_AlignHandle, osPriorityLow6);
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
        if (systemState == STATE_UV_LED_ALIGN)  // UV LED align command
        {
            // Send acknowledgment
            HAL_UART_Transmit(&huart1, (uint8_t *)"UV LED ALIGN START\r\n", 21, HAL_MAX_DELAY);

            Stepper_UV_Sensor_Align();

            HAL_UART_Transmit(&huart1, (uint8_t *)"UV LED ALIGN COMPLETE\r\n", 24, HAL_MAX_DELAY);

            // Return to IDLE
            systemState = STATE_IDLE;
            osThreadSetPriority(UV_Led_AlignHandle, osPriorityLow7);
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

    for(;;)
    {
        if (systemState == STATE_ASPIRATE_SAMPLE)
        {

            HAL_UART_Transmit(
                &huart1,
                (uint8_t *)"ASPIRATE SAMPLE START\r\n",
                24,
                HAL_MAX_DELAY
            );

            osMutexAcquire(pumpMutexHandle, osWaitForever);

//            PUMP_Move(
//                PUMP_FORWARD,
//                1000, //DURATION SEC
//                55 //PWM PERCENTAGE
//            );

            PUMP_Move(
                PUMP_FORWARD,
                1000, //DURATION SEC
                55 //PWM PERCENTAGE
            );

            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
            osDelay(100);
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);


            osDelay(2000);

            PUMP_Move(
            	PUMP_FORWARD,
                500, //DURATION SEC
                50 //PWM PERCENTAGE
            );

//            PUMP_Move(
//                PUMP_REVERSE,
//                5,
//                75
//            );

            PUMP_Stop();

            osMutexRelease(pumpMutexHandle);

            HAL_UART_Transmit(
                &huart1,
                (uint8_t *)"ASPIRATE SAMPLE COMPLETE\r\n",
                26,
                HAL_MAX_DELAY
            );

            uartCmd.start = 0;
            uartCmd.sensor = SENSOR_NONE;
            uartCmd.duration = 0;
            uartCmd.incubation = 0;

            systemState = STATE_IDLE;

            osThreadSetPriority(
                AspirateHandle,
                osPriorityNormal1
            );
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
//  uint16_t command;
  /* Infinite loop */
  for(;;)
  {
      switch (systemState)
      {
          case STATE_IDLE:

              /*
               * =====================================================
               * 1. LIMIT SWITCH HAS PRIORITY WHEN SYSTEM IS IDLE
               * =====================================================
               */
              if (limitSwitchPressed)
              {
                  /*
                   * Consume the event immediately.
                   */
                  limitSwitchPressed = 0;

                  /*
                   * Clear any old command data.
                   */
                  uartCmd.sensor = SENSOR_NONE;
                  uartCmd.duration = 0;
                  uartCmd.incubation = 0;
                  uartCmd.start = 0;

                  /*
                   * Start aspiration.
                   */
                  systemState = STATE_ASPIRATE_SAMPLE;

                  /*
                   * Wake / prioritize aspiration task.
                   */
                  osThreadSetPriority(
                      AspirateHandle,
                      osPriorityRealtime
                  );

                  /*
                   * Tell RPi that aspiration was triggered
                   * by the limit switch.
                   */
                  HAL_UART_Transmit(
                      &huart1,
                      (uint8_t *)"LIMIT SWITCH -> ASPIRATE\r\n",
                      26,
                      HAL_MAX_DELAY
                  );
              }

              /*
               * =====================================================
               * 2. NORMAL RPI COMMAND PROCESSING
               * =====================================================
               */
              else if (uartCmd.start != 0)
              {
                  switch (uartCmd.start)
                  {
                      case 1:
                          /*
                           * Sensor command
                           */
                          if (uartCmd.sensor == SENSOR_AS7341)
                          {
                              systemState = STATE_AS7341_MEASURE;

                              osThreadSetPriority(
                                  AS7341_SendHandle,
                                  osPriorityRealtime
                              );
                          }
                          else if (uartCmd.sensor == SENSOR_LTR390)
                          {
                              systemState = STATE_LTR390_MEASURE;

                              osThreadSetPriority(
                                  Send_LTR390Handle,
                                  osPriorityRealtime
                              );
                          }
                          break;


                      case 2:
                          /*
                           * Stepper home
                           */
                          systemState = STATE_STEPPER_HOME;

                          osThreadSetPriority(
                              Stepper_HomeHandle,
                              osPriorityRealtime
                          );
                          break;


                      case 3:
                          /*
                           * White LED alignment
                           */
                          systemState = STATE_WHITE_LED_ALIGN;

                          osThreadSetPriority(
                              White_Led_AlignHandle,
                              osPriorityRealtime
                          );
                          break;


                      case 4:
                          /*
                           * UV LED alignment
                           */
                          systemState = STATE_UV_LED_ALIGN;

                          osThreadSetPriority(
                              UV_Led_AlignHandle,
                              osPriorityRealtime
                          );
                          break;


                      case 5:
                          /*
                           * Manual RPi aspiration command
                           */
                          systemState = STATE_ASPIRATE_SAMPLE;

                          osThreadSetPriority(
                              AspirateHandle,
                              osPriorityRealtime
                          );
                          break;

                       case 6:
                          /*
                           * Flow cell cleaning
                           */
                          systemState = STATE_FLOW_CELL_CLEAN;

                          osThreadSetPriority(
                              Clean_Flow_CellHandle,
                              osPriorityRealtime
                          );

                          break;


                      default:
                          break;
                  }

                  /*
                   * Command has now been consumed.
                   */
                  uartCmd.start = 0;
              }

              break;


          /*
           * =========================================================
           * BUSY STATES
           * =========================================================
           *
           * These operations are handled by their own threads.
           *
           * Limit switch does NOT interrupt them.
           */
          case STATE_WHITE_LED_ALIGN:

          case STATE_UV_LED_ALIGN:

          case STATE_STEPPER_HOME:

          case STATE_ASPIRATE_SAMPLE:

          case STATE_AS7341_MEASURE:

          case STATE_LTR390_MEASURE:

          case STATE_FLOW_CELL_CLEAN:

              /*
               * Do nothing.
               *
               * The corresponding task owns this state.
               *
               * Limit switch presses during these states
               * are ignored by Limit_sw_pressed().
               */
              break;


          default:
              systemState = STATE_IDLE;
              break;
      }

      osDelay(10);
  }
  /* USER CODE END Command_Dispatcher */
}

/* USER CODE BEGIN Header_Limit_sw_pressed */
/**
* @brief Function implementing the Limit_sw thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Limit_sw_pressed */
void Limit_sw_pressed(void *argument)
{
  /* USER CODE BEGIN Limit_sw_pressed */
	uint8_t lastState = 0;
  /* Infinite loop */
  for(;;)
  {
      uint8_t currentState =
          HAL_GPIO_ReadPin(Limit_SW_GPIO_Port, Limit_SW_Pin);

      /*
       * Detect NEW physical press
       */
      if (currentState && !lastState)
      {
          /*
           * IMPORTANT:
           * Capture whether the system was IDLE
           * at the exact moment the switch was pressed.
           */
          uint8_t acceptPress = (systemState == STATE_IDLE);

          /*
           * Debounce
           */
          osDelay(50);

          /*
           * Verify switch is still pressed
           */
          currentState =
              HAL_GPIO_ReadPin(
                  Limit_SW_GPIO_Port,
                  Limit_SW_Pin
              );

          /*
           * Only generate event if:
           *
           * 1. It was IDLE when physically pressed
           * 2. Switch is still pressed after debounce
           */
          if (currentState && acceptPress)
          {
              limitSwitchPressed = 1;
          }
      }

      /*
       * Remember physical switch state.
       *
       * This guarantees one event per physical press.
       */
      lastState = currentState;

      osDelay(10);
  }
  /* USER CODE END Limit_sw_pressed */
}

/* USER CODE BEGIN Header_Flow_Cell_Clean */
/**
* @brief Function implementing the Clean_Flow_Cell thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Flow_Cell_Clean */
void Flow_Cell_Clean(void *argument)
{
  /* USER CODE BEGIN Flow_Cell_Clean */

  /* Infinite loop */
  for(;;)
  {
      if (systemState == STATE_FLOW_CELL_CLEAN)
      {

          HAL_UART_Transmit(
              &huart1,
              (uint8_t *)"FLOW CELL CLEAN START\r\n",
              23,
              HAL_MAX_DELAY
          );

          osMutexAcquire(pumpMutexHandle, osWaitForever);


          PUMP_Move(
              PUMP_FORWARD,
              2000,      // seconds
              80      // PWM %
          );

          PUMP_Stop();

          osMutexRelease(pumpMutexHandle);


          HAL_UART_Transmit(
              &huart1,
              (uint8_t *)"FLOW CELL CLEAN COMPLETE\r\n",
              27,
              HAL_MAX_DELAY
          );


          uartCmd.start = 0;
          uartCmd.sensor = SENSOR_NONE;
          uartCmd.duration = 0;
          uartCmd.incubation = 0;

          systemState = STATE_IDLE;


          osThreadSetPriority(
              Clean_Flow_CellHandle,
              osPriorityLow
          );
      }

      osDelay(10);
  }
  /* USER CODE END Flow_Cell_Clean */
}

/* USER CODE BEGIN Header_pump_initialize */
/**
* @brief Function implementing the initialize_pump thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_pump_initialize */
void pump_initialize(void *argument)
{
  /* USER CODE BEGIN pump_initialize */
  /* Infinite loop */
  for(;;)
  {
      if (initializePumpRequested)
      {
          initializePumpRequested = 0;

          HAL_UART_Transmit(
              &huart1,
              (uint8_t *)"PUMP INITIALIZATION START\r\n",
              29,
              HAL_MAX_DELAY
          );

          osMutexAcquire(pumpMutexHandle, osWaitForever);

          PUMP_Move(
              PUMP_FORWARD,
              1000,
              80
          );

          PUMP_Stop();

          osMutexRelease(pumpMutexHandle);

          HAL_UART_Transmit(
              &huart1,
              (uint8_t *)"PUMP INITIALIZATION COMPLETE\r\n",
              32,
              HAL_MAX_DELAY
          );
      }

      osDelay(10);
  }
  /* USER CODE END pump_initialize */
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
