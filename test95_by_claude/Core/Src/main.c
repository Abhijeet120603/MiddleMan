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

#include <stdarg.h>  /* Add this for variadic functions */
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

#define RX_BUFFER_SIZE 64

/* Add this for mutex timeouts */
#define MUTEX_TIMEOUT_MS    100
#define MUTEX_TIMEOUT_TICKS (MUTEX_TIMEOUT_MS * configTICK_RATE_HZ / 1000)
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
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UartSend */
osThreadId_t UartSendHandle;
const osThreadAttr_t UartSend_attributes = {
  .name = "UartSend",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Stepper_Home */
osThreadId_t Stepper_HomeHandle;
const osThreadAttr_t Stepper_Home_attributes = {
  .name = "Stepper_Home",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for White_Led_Align */
osThreadId_t White_Led_AlignHandle;
const osThreadAttr_t White_Led_Align_attributes = {
  .name = "White_Led_Align",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UV_Led_Align */
osThreadId_t UV_Led_AlignHandle;
const osThreadAttr_t UV_Led_Align_attributes = {
  .name = "UV_Led_Align",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Aspirate */
osThreadId_t AspirateHandle;
const osThreadAttr_t Aspirate_attributes = {
  .name = "Aspirate",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Command_Dispatc */
osThreadId_t Command_DispatcHandle;
const osThreadAttr_t Command_Dispatc_attributes = {
  .name = "Command_Dispatc",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Limit_sw */
osThreadId_t Limit_swHandle;
const osThreadAttr_t Limit_sw_attributes = {
  .name = "Limit_sw",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Clean_Flow_Cell */
osThreadId_t Clean_Flow_CellHandle;
const osThreadAttr_t Clean_Flow_Cell_attributes = {
  .name = "Clean_Flow_Cell",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for initialize_pump */
osThreadId_t initialize_pumpHandle;
const osThreadAttr_t initialize_pump_attributes = {
  .name = "initialize_pump",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Reset */
osThreadId_t ResetHandle;
const osThreadAttr_t Reset_attributes = {
  .name = "Reset",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for uartTxQueue */
osMessageQueueId_t uartTxQueueHandle;
const osMessageQueueAttr_t uartTxQueue_attributes = {
  .name = "uartTxQueue"
};
/* Definitions for uartRxQueue */
osMessageQueueId_t uartRxQueueHandle;
const osMessageQueueAttr_t uartRxQueue_attributes = {
  .name = "uartRxQueue"
};
/* Definitions for execCommandQueue */
osMessageQueueId_t execCommandQueueHandle;
const osMessageQueueAttr_t execCommandQueue_attributes = {
  .name = "execCommandQueue"
};
/* Definitions for pumpMutex */
osMutexId_t pumpMutexHandle;
const osMutexAttr_t pumpMutex_attributes = {
  .name = "pumpMutex"
};
/* Definitions for uart1Mutex */
osMutexId_t uart1MutexHandle;
const osMutexAttr_t uart1Mutex_attributes = {
  .name = "uart1Mutex"
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
volatile char rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;

/* Remove: volatile uint8_t commandReady = 0; */

// volatile UART_Command_t uartCmd = {0};  // REMOVE THIS - replaced by execution queue

uint8_t AS7341_Present = 0;

volatile SystemState_t systemState = STATE_IDLE;
volatile uint8_t limitSwitchPressed = 0;
/* Set by the ABORT command: every worker must unwind to a safe state at its
   next checkpoint instead of running to completion. Cleared by the worker
   that observes it, or by the dispatcher when starting a fresh command. */
volatile uint8_t abortRequested = 0;
/* Counts UART ISR faults (overrun/noise/framing). Reported and cleared by
   the dispatcher — never printed from ISR context. */
volatile uint32_t uartErrorPending = 0;
/* Asks the RX ISR to drop a half-received line (set by ABORT handling and
   by the error path). Prevents a corrupt fragment from gluing onto the
   next valid command and making it UNKNOWN. */
volatile uint8_t uartRxFlushReq = 0;

volatile uint8_t initializePumpRequested = 0;

/* RX queue overflow counter for debugging */
volatile uint32_t uartRxQueueOverflow = 0;
volatile uint32_t uartTxQueueFull = 0;

/* UART TX Message Structure */
#define UART_TX_MSG_SIZE 128

typedef struct
{
    uint16_t length;
    uint8_t data[UART_TX_MSG_SIZE];
} UART_TxMessage_t;

/* RX Command Queue Structure */
#define RX_COMMAND_QUEUE_SIZE 10
#define RX_COMMAND_SIZE       64

typedef struct
{
    char command[RX_COMMAND_SIZE];
} UART_CommandMessage_t;

/* Execution Command Queue Structure - stores actual command parameters */
#define EXEC_COMMAND_QUEUE_SIZE 10

typedef struct
{
    uint8_t sensor;
    uint32_t duration;
    uint32_t incubation;
    uint8_t start;      /* 1=sensor, 2=stepper, 3=white, 4=uv, 5=aspirate, 6=clean */
} UART_ExecutionCommand_t;

volatile UART_ExecutionCommand_t uartCmdTemp = {0};
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
void reset(void *argument);

/* USER CODE BEGIN PFP */
void ControlWhiteLED(uint8_t state);

void UART_Send(const char *message);
void UART_SendFormatted(const char *format, ...);
void ProcessUARTCommand(const char *command);
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

  ControlWhiteLED(LED_ON);   // WHITE LED ALWAYS ON

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of pumpMutex */
  pumpMutexHandle = osMutexNew(&pumpMutex_attributes);

  /* creation of uart1Mutex */
  uart1MutexHandle = osMutexNew(&uart1Mutex_attributes);

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
  /* creation of uartTxQueue */
  uartTxQueueHandle = osMessageQueueNew (20, sizeof(uint16_t), &uartTxQueue_attributes);

  /* creation of uartRxQueue */
  uartRxQueueHandle = osMessageQueueNew (10, sizeof(uint16_t), &uartRxQueue_attributes);

  /* creation of execCommandQueue */
  execCommandQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &execCommandQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  uartTxQueueHandle = osMessageQueueNew(
      20,
      sizeof(UART_TxMessage_t),
      &uartTxQueue_attributes
  );

  uartRxQueueHandle = osMessageQueueNew(
      RX_COMMAND_QUEUE_SIZE,
      sizeof(UART_CommandMessage_t),
      &uartRxQueue_attributes
  );

  execCommandQueueHandle = osMessageQueueNew(
      EXEC_COMMAND_QUEUE_SIZE,
      sizeof(UART_ExecutionCommand_t),
      &execCommandQueue_attributes
  );
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

  /* creation of Reset */
  ResetHandle = osThreadNew(reset, NULL, &Reset_attributes);

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

  /*Configure GPIO pin : Reset_Pin */
  GPIO_InitStruct.Pin = Reset_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Reset_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static char cmdBuffer[RX_COMMAND_SIZE];
    static uint8_t cmdIndex = 0;
    osStatus_t status;

    if (huart->Instance == USART1)
    {
        /* ABORT / error recovery asked us to drop a half-received line */
        if (uartRxFlushReq)
        {
            uartRxFlushReq = 0;
            cmdIndex = 0;
            memset(cmdBuffer, 0, sizeof(cmdBuffer));
        }

        if (rxByte == '\r' || rxByte == '\n')
        {
            if (cmdIndex > 0)
            {
                cmdBuffer[cmdIndex] = '\0';

                /* Trim trailing newline if present */
                if (cmdBuffer[cmdIndex - 1] == '\n')
                {
                    cmdBuffer[cmdIndex - 1] = '\0';
                    cmdIndex--;
                }

                if (cmdIndex > 0)
                {
                    UART_CommandMessage_t msg;
                    strncpy(msg.command, cmdBuffer, RX_COMMAND_SIZE - 1);
                    msg.command[RX_COMMAND_SIZE - 1] = '\0';

                    /* Put command into RX queue - non-blocking from ISR */
                    status = osMessageQueuePut(uartRxQueueHandle, &msg, 0, 0);

                    if (status != osOK)
                    {
                        /* RX queue full - increment overflow counter */
                        uartRxQueueOverflow++;

                        /* Debug: Queue full */
                        // UART_Send("debug0: RX Queue Full\r\n");
                    }
                    else
                    {
                        /* Debug: Command queued successfully */
                        // UART_Send("debug1: RX Command Queued\r\n");
                    }
                }

                cmdIndex = 0;
            }
        }
        else
        {
            if (cmdIndex < RX_COMMAND_SIZE - 1)
            {
                cmdBuffer[cmdIndex++] = rxByte;
            }
            else
            {
                /* Buffer overflow - reset */
                cmdIndex = 0;
                memset(cmdBuffer, 0, sizeof(cmdBuffer));

                /* Debug: Buffer overflow */
                // UART_Send("debug2: RX Buffer Overflow\r\n");
            }
        }

        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rxByte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* ISR CONTEXT — never block, never queue messages, never abort an
           in-flight transfer from here. Just record the fault, clear the
           sticky hardware flags, drop any half-received line and re-arm
           reception. The dispatcher reports the fault when idle. */
        uartErrorPending++;
        uartRxFlushReq = 1;

        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_UART_FLUSH_DRREGISTER(huart);

        huart->ErrorCode = HAL_UART_ERROR_NONE;
        huart->RxState = HAL_UART_STATE_READY;

        /* Reinitialize the receive interrupt */
        HAL_UART_Receive_IT(huart, &rxByte, 1);
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

void ProcessUARTCommand(const char *command)
{
    UART_ExecutionCommand_t execCmd = {0};
    osStatus_t status;

    /* ABORT: cancel whatever is running and park in IDLE.
       Runs in dispatcher context. Signals the owning worker via
       abortRequested, drops everything queued behind the aborted op, parks
       the pump/LEDs/buzzer safe, then WAITS (max 3 s) for the worker to
       unwind before answering — so the PC never mistakes a late COMPLETE
       for the next command's reply. */
    if (strcmp(command, "ABORT") == 0 || strcmp(command, "abort") == 0)
    {
        abortRequested = 1;

        /* Drop anything queued behind the aborted operation */
        {
            UART_ExecutionCommand_t dummyExec;
            while (osMessageQueueGet(execCommandQueueHandle, &dummyExec, NULL, 0) == osOK);
        }
        {
            UART_CommandMessage_t dummyRx;
            while (osMessageQueueGet(uartRxQueueHandle, &dummyRx, NULL, 0) == osOK);
        }

        /* Ask the RX ISR to drop a half-received line */
        uartRxFlushReq = 1;

        /* Leave hardware in a safe state */
        PUMP_Stop();
        ControlWhiteLED(LED_OFF);
        HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);

        /* Wait for the owning worker to unwind to IDLE */
        {
            uint32_t waitedMs = 0;
            while (systemState != STATE_IDLE && waitedMs < 3000)
            {
                osDelay(10);
                waitedMs += 10;
            }
        }

        if (systemState == STATE_IDLE)
            UART_Send("ABORT COMPLETE\r\n");
        else
            UART_Send("ABORT TIMEOUT\r\n");
        return;
    }

    /* Check for new commands */
    if (strcmp(command, "PING") == 0)
    {
        UART_Send("PONG\r\n");
        initializePumpRequested = 1;
        return;
    }
    else if (strcmp(command, "HELP") == 0 || strcmp(command, "help") == 0)
    {
        UART_Send(
            "Available commands:\r\n"
            "PING - Test connection\r\n"
            "ABORT - Cancel active operation\r\n"
            "STEPPER_HOME - Home the stepper motor\r\n"
            "WHITE_LED_ALIGN - Start white LED alignment\r\n"
            "UV_LED_ALIGN - Start UV LED alignment\r\n"
            "ASPIRATE - Run aspirate sample sequence\r\n"
            "AS7341,<duration>,<incubation> - AS7341 sensor measurement\r\n"
            "LTR390,<duration>,<incubation> - LTR390 sensor measurement\r\n"
            "LIMIT_SWITCH - Automatically triggers aspiration when IDLE\r\n"
            "CLEAN - Run flow cell cleaning\r\n"
        );
        return;
    }
    else if (strcmp(command, "STEPPER_HOME") == 0 || strcmp(command, "stepper_home") == 0)
    {
        execCmd.sensor = SENSOR_NONE;
        execCmd.duration = 0;
        execCmd.incubation = 0;
        execCmd.start = 2;  /* Use 2 for stepper home */
    }
    else if (strcmp(command, "WHITE_LED_ALIGN") == 0 || strcmp(command, "white_led_align") == 0)
    {
        execCmd.sensor = SENSOR_NONE;
        execCmd.duration = 0;
        execCmd.incubation = 0;
        execCmd.start = 3;  /* Use 3 for white LED align */
    }
    else if (strcmp(command, "UV_LED_ALIGN") == 0 || strcmp(command, "uv_led_align") == 0)
    {
        execCmd.sensor = SENSOR_NONE;
        execCmd.duration = 0;
        execCmd.incubation = 0;
        execCmd.start = 4;  /* Use 4 for UV LED align */
    }
    else if (strcmp(command, "ASPIRATE") == 0 || strcmp(command, "aspirate") == 0 ||
             strcmp(command, "ASPIRATE_SAMPLE") == 0)
    {
        execCmd.sensor = SENSOR_NONE;
        execCmd.duration = 0;
        execCmd.incubation = 0;
        execCmd.start = 5;  /* Use 5 for aspirate sample */
    }
    else if (strcmp(command, "CLEAN") == 0 || strcmp(command, "clean") == 0)
    {
        /* REMOVED: systemState == STATE_IDLE check - always queue CLEAN */
        execCmd.sensor = SENSOR_NONE;
        execCmd.duration = 0;
        execCmd.incubation = 0;
        execCmd.start = 6;  /* Use 6 for CLEAN command */

        UART_Send("CLEAN COMMAND RECEIVED\r\n");
    }
    else
    {
        /* Existing sensor command parsing */
        char sensor[20];
        int sec;
        int incubate;

        /* Try parsing with 3 parameters: sensor,duration,incubation */
        if (sscanf(command, "%[^,],%d,%d", sensor, &sec, &incubate) == 3)
        {
            if (strcmp(sensor, "AS7341") == 0)
            {
                execCmd.sensor = SENSOR_AS7341;
                execCmd.duration = sec;
                execCmd.incubation = incubate;
                execCmd.start = 1;
            }
            else if (strcmp(sensor, "LTR390") == 0)
            {
                execCmd.sensor = SENSOR_LTR390;
                execCmd.duration = sec;
                execCmd.incubation = incubate;
                execCmd.start = 1;
            }
            else
            {
                UART_Send("UNKNOWN COMMAND\r\n");
                return;
            }
        }
        /* Fallback to 2 parameters for backward compatibility */
        else if (sscanf(command, "%[^,],%d", sensor, &sec) == 2)
        {
            if (strcmp(sensor, "AS7341") == 0)
            {
                execCmd.sensor = SENSOR_AS7341;
                execCmd.duration = sec;
                execCmd.incubation = 0;  /* No incubation */
                execCmd.start = 1;
            }
            else if (strcmp(sensor, "LTR390") == 0)
            {
                execCmd.sensor = SENSOR_LTR390;
                execCmd.duration = sec;
                execCmd.incubation = 0;  /* No incubation */
                execCmd.start = 1;
            }
            else
            {
                UART_Send("UNKNOWN COMMAND\r\n");
                return;
            }
        }
        else
        {
            UART_Send("UNKNOWN COMMAND\r\n");
            return;
        }
    }

    /* Put the execution command into the execution queue */
    status = osMessageQueuePut(execCommandQueueHandle, &execCmd, 0, 0);

    if (status != osOK)
    {
        /* Execution queue full - commands are being lost */
        UART_Send("ERROR: EXECUTION QUEUE FULL\r\n");

        /* Debug: Execution queue full */
        // UART_Send("debug16: Exec Queue Full\r\n");
    }
    else
    {
        /* Debug: Command queued for execution */
        // UART_Send("debug17: Exec Command Queued\r\n");
    }

    UART_Send("\r\n");
}


void UART_Send(const char *message)
{
    UART_TxMessage_t msg;
    uint16_t len = strlen(message);
    osStatus_t status;

    if (len >= UART_TX_MSG_SIZE)
        len = UART_TX_MSG_SIZE - 1;

    msg.length = len;
    memcpy(msg.data, message, len);

    /* Use timeout to avoid silent message loss */
    status = osMessageQueuePut(uartTxQueueHandle, &msg, 0, MUTEX_TIMEOUT_TICKS);

    if (status != osOK)
    {
        /* TX queue full - increment counter */
        uartTxQueueFull++;

        /* Debug: Queue full */
        // UART_Send("debug3: TX Queue Full\r\n");

        /* Option: Block until space available */
        /* status = osMessageQueuePut(uartTxQueueHandle, &msg, 0, osWaitForever); */
    }
    else
    {
        /* Debug: Message queued successfully */
        // UART_Send("debug4: TX Message Queued\r\n");
    }
}

void UART_SendFormatted(const char *format, ...)
{
    char buffer[UART_TX_MSG_SIZE];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    UART_Send(buffer);
}

/* FreeRTOS safety hooks (enabled via configCHECK_FOR_STACK_OVERFLOW /
   configUSE_MALLOC_FAILED_HOOK). A stack overflow used to corrupt memory
   silently and the box died mysteriously after N tests. Now: best-effort
   one-line report (non-blocking queue put — safe even with the scheduler
   disturbed) + short busy wait so the byte gets out + software reset.
   The PC side sees "FW BOOT READY" right after and can tell it was a
   firmware reset, not a GUI stall. */
static void FW_FatalReport(const char *message)
{
    UART_TxMessage_t msg;
    uint16_t len = strlen(message);

    if (len >= UART_TX_MSG_SIZE)
        len = UART_TX_MSG_SIZE - 1;
    msg.length = len;
    memcpy(msg.data, message, len);
    /* Zero timeout: never block, even if the scheduler is disturbed. */
    (void)osMessageQueuePut(uartTxQueueHandle, &msg, 0, 0);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    FW_FatalReport("FATAL: STACK OVERFLOW\r\n");
    for (volatile uint32_t spin = 0; spin < 8000000UL; spin++) { __NOP(); }
    NVIC_SystemReset();
    for (;;) { }
}

void vApplicationMallocFailedHook(void)
{
    FW_FatalReport("FATAL: HEAP EXHAUSTED\r\n");
    for (volatile uint32_t spin = 0; spin < 8000000UL; spin++) { __NOP(); }
    NVIC_SystemReset();
    for (;;) { }
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
          osDelay(5);
          HAL_I2C_Init(&hi2c1);

          if (HAL_I2C_IsDeviceReady(&hi2c1,
                                    AS7341_I2CADDR_DEFAULT << 1,
                                    5,
									MUTEX_TIMEOUT_TICKS) != HAL_OK)
          {
        	  UART_Send("AS7341 NOT FOUND\r\n");

              systemState = STATE_IDLE;
              osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
              continue;
          }

          Adafruit_AS7341_Init(&as7341);

          if (!Adafruit_AS7341_begin(&as7341,
                                     AS7341_I2CADDR_DEFAULT,
                                     &hi2c1,
                                     0))
          {
        	  UART_Send("AS7341 INIT FAILED\r\n");

              systemState = STATE_IDLE;
              osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
              continue;
          }

          /* Sensor is connected - initialize it again */
          Adafruit_AS7341_Init(&as7341);

          if (!Adafruit_AS7341_begin(&as7341,
                                     AS7341_I2CADDR_DEFAULT,
                                     &hi2c1,
                                     0))
          {
        	  UART_Send("AS7341 INIT FAILED\r\n");

              systemState = STATE_IDLE;
              osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
              continue;
          }

          Adafruit_AS7341_setATIME(&as7341, 50);
          Adafruit_AS7341_setASTEP(&as7341, 999);
          Adafruit_AS7341_setGain(&as7341, AS7341_GAIN_16X);

          // Ensure LED is off initially
//          ControlWhiteLED(LED_OFF);

          // Incubation period in 1 s chunks so ABORT is honoured promptly
          // (same total wait as before; white-LED control stays disabled)
          for (uint32_t inc = 0; inc < uartCmdTemp.incubation; inc++)
          {
              if (abortRequested)
              {
                  break;
              }
              osDelay(1000);
          }
          if (abortRequested)
          {
              uartCmdTemp.start = 0;
              uartCmdTemp.sensor = SENSOR_NONE;
              /* Never clobber a newer operation that may have started */
              if (systemState == STATE_AS7341_MEASURE || systemState == STATE_IDLE)
              {
                  systemState = STATE_IDLE;
              }
              abortRequested = 0;
              osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
              continue;
          }

          // Start measurements
          uint32_t tick = osKernelGetTickCount();

          for (uint32_t i = 0; i < uartCmdTemp.duration; i++)
          {
              /* Abort checkpoint: stop streaming immediately on Back */
              if (abortRequested)
              {
                  break;
              }

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

                  UART_Send(tx);
              }

              tick += 1000;
              osDelayUntil(tick);
          }

          // Turn off LED after measurements complete
//          ControlWhiteLED(LED_OFF);

          // Reset command and return to IDLE
          // (never clobber a newer operation that may have started)
          uartCmdTemp.start = 0;
          uartCmdTemp.sensor = SENSOR_NONE;
          if (systemState == STATE_AS7341_MEASURE || systemState == STATE_IDLE)
          {
              systemState = STATE_IDLE;
          }
          abortRequested = 0;
          osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
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
									MUTEX_TIMEOUT_TICKS) != HAL_OK)
          {
        	  UART_Send("LTR390 NOT FOUND\r\n");

              systemState = STATE_IDLE;
              osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
              continue;
          }

          /* Reinitialize every time */
          LTR390_Init(&ltr, &hi2c3);

          if (!LTR390_Begin(&ltr))
          {
        	  UART_Send("LTR390 INIT FAILED\r\n");

              systemState = STATE_IDLE;
              osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
              continue;
          }

          LTR390_SetMode(&ltr, LTR390_MODE_UVS);
          LTR390_SetGain(&ltr, LTR390_GAIN_18);
          LTR390_SetResolution(&ltr, LTR390_RESOLUTION_20BIT);

          // Wait for first 20-bit UV conversion
          osDelay(450);

          // Ensure UV LED is off initially
          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

          // Incubation period handling with LED control (abort-aware, but
          // with identical LED timing: UV LED on for the final 1 s only)
          if (uartCmdTemp.incubation > 0)
          {
              if (uartCmdTemp.incubation > 1)
              {
                  // Wait until 1 second before incubation ends
                  for (uint32_t inc = 0; inc < uartCmdTemp.incubation - 1; inc++)
                  {
                      if (abortRequested)
                      {
                          break;
                      }
                      osDelay(1000);
                  }

                  // Turn on UV LED
                  if (!abortRequested)
                  {
                      HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);
                  }

                  // Wait for the remaining 1 second
                  if (!abortRequested)
                  {
                      osDelay(1000);
                  }
              }
              else
              {
                  // Incubation is 1 second or less - turn on UV LED immediately
                  HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);

                  // Wait for incubation time
                  if (!abortRequested)
                  {
                      osDelay(1000);
                  }
              }
          }
          else
          {
              // No incubation - turn on UV LED immediately
              HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);
          }
          if (abortRequested)
          {
              HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);
              uartCmdTemp.start = 0;
              uartCmdTemp.sensor = SENSOR_NONE;
              /* Never clobber a newer operation that may have started */
              if (systemState == STATE_LTR390_MEASURE || systemState == STATE_IDLE)
              {
                  systemState = STATE_IDLE;
              }
              abortRequested = 0;
              osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
              continue;
          }

          // Start measurements
          uint32_t tick = osKernelGetTickCount();

          for (uint32_t i = 0; i < uartCmdTemp.duration; i++)
          {
              /* Abort checkpoint: stop streaming immediately on Back */
              if (abortRequested)
              {
                  break;
              }

              if (LTR390_NewDataAvailable(&ltr))
              {
                  uv340 = LTR390_ReadUVS(&ltr);

                  char tx[30];
                  snprintf(tx,
                           sizeof(tx),
                           "%lu,%lu\r\n",
                           i + 1,
                           uv340);

                  UART_Send(tx);
              }

              tick += 1000;
              osDelayUntil(tick);
          }

          // Turn off UV LED after measurements complete
          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

          // Reset command and return to IDLE
          // (never clobber a newer operation that may have started)
          uartCmdTemp.start = 0;
          uartCmdTemp.sensor = SENSOR_NONE;
          if (systemState == STATE_LTR390_MEASURE || systemState == STATE_IDLE)
          {
              systemState = STATE_IDLE;
          }
          abortRequested = 0;
          osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
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
	UART_TxMessage_t msg;

    for (;;)
    {
        if (osMessageQueueGet(uartTxQueueHandle, &msg, NULL, osWaitForever) == osOK)
        {
            if (osMutexAcquire(uart1MutexHandle, MUTEX_TIMEOUT_TICKS) == osOK)
            {
                HAL_UART_Transmit(
                    &huart1,
                    msg.data,
                    msg.length,
                    1000
                );

                osMutexRelease(uart1MutexHandle);
            }
            else
            {
                /* Could not acquire UART mutex - try to resend later */
                /* Or just drop the message to avoid blocking */
            }
        }
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
        	UART_Send("STEPPER HOME START\r\n");

            // Call your stepper home function (abort-aware, see Stepper.c)
            Stepper_Home();


            if (abortRequested)
            {
                UART_Send("STEPPER HOME ABORTED\r\n");
            }
            else
            {
                UART_Send("STEPPER HOME COMPLETE\r\n");
            }

            // Return to IDLE (never clobber a newer operation)
            if (systemState == STATE_STEPPER_HOME || systemState == STATE_IDLE)
            {
                systemState = STATE_IDLE;
            }
            abortRequested = 0;
            osThreadSetPriority(Stepper_HomeHandle, osPriorityNormal);
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
    	  UART_Send("WHITE LED ALIGN START\r\n");

        // Perform alignment (abort-aware, see Stepper.c)

        Stepper_White_LED_Align();


        if (abortRequested)
        {
            UART_Send("WHITE LED ALIGN ABORTED\r\n");
        }
        else
        {
            UART_Send("WHITE LED ALIGN COMPLETE\r\n");
        }

        // Return to IDLE (never clobber a newer operation)
        if (systemState == STATE_WHITE_LED_ALIGN || systemState == STATE_IDLE)
        {
            systemState = STATE_IDLE;
        }
        abortRequested = 0;
        osThreadSetPriority(White_Led_AlignHandle, osPriorityNormal);
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
        	UART_Send("UV LED ALIGN START\r\n");

            Stepper_UV_Sensor_Align();

            if (abortRequested)
            {
                UART_Send("UV LED ALIGN ABORTED\r\n");
            }
            else
            {
                UART_Send("UV LED ALIGN COMPLETE\r\n");
            }

            // Return to IDLE (never clobber a newer operation)
            if (systemState == STATE_UV_LED_ALIGN || systemState == STATE_IDLE)
            {
                systemState = STATE_IDLE;
            }
            abortRequested = 0;
            osThreadSetPriority(UV_Led_AlignHandle, osPriorityNormal);
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

        	UART_Send("ASPIRATE SAMPLE START\r\n");

            PUMP_Move(
                PUMP_FORWARD,
                1000, //DURATION SEC
                55 //PWM PERCENTAGE
            );

            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
            osDelay(100);
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);

            /* Was a single osDelay(2000): split into checkpoints so ABORT
               unwinds fast instead of sleeping through the cancel. */
            for (uint32_t aspWait = 0; aspWait < 20; aspWait++)
            {
                if (abortRequested)
                {
                    break;
                }
                osDelay(100);
            }

            PUMP_Move(
            	PUMP_FORWARD,
                500, //DURATION SEC
                50 //PWM PERCENTAGE
            );

            PUMP_Stop();

            if (abortRequested)
            {
                UART_Send("ASPIRATE SAMPLE ABORTED\r\n");
            }
            else
            {
                UART_Send("ASPIRATE SAMPLE COMPLETE\r\n");
            }

            uartCmdTemp.start = 0;
            uartCmdTemp.sensor = SENSOR_NONE;
            uartCmdTemp.duration = 0;
            uartCmdTemp.incubation = 0;

            /* Never clobber a newer operation that may have started */
            if (systemState == STATE_ASPIRATE_SAMPLE || systemState == STATE_IDLE)
            {
                systemState = STATE_IDLE;
            }
            abortRequested = 0;

            osThreadSetPriority(
                AspirateHandle,
				osPriorityNormal
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
    UART_CommandMessage_t cmdMsg;
    UART_ExecutionCommand_t execCmd;
    osStatus_t status;
  /* Infinite loop */
  for(;;)
  {
      /* Boot banner (once): proves the firmware (re)started. The PC log
         uses it to distinguish a firmware reset from a GUI-side stall. */
      {
          static uint8_t bootAnnounced = 0;
          if (!bootAnnounced)
          {
              bootAnnounced = 1;
              UART_Send("FW BOOT READY\r\n");
          }
      }

      /* Check if a command is available from RX queue */
      status = osMessageQueueGet(uartRxQueueHandle, &cmdMsg, NULL, 0);

      /* Debug: Check queue status */
      // if (status == osOK)
      // {
      //     UART_Send("debug5: Command Retrieved\r\n");
      // }

      if (status == osOK)
      {
          /* Debug: About to process command */
          // UART_Send("debug6: Processing Command\r\n");

          /* Process the command - this puts it into the execution queue */
          ProcessUARTCommand(cmdMsg.command);

          /* Debug: Command processed */
          // UART_Send("debug7: Command Processed\r\n");
      }

      /* === STATE MACHINE PROCESSING === */
      switch (systemState)
      {
          case STATE_IDLE:
              /* Report (once) any UART ISR faults seen since the last
                 report, then clear the counter. Visible in the GUI log so
                 recurring line faults can be diagnosed instead of showing
                 up as mystery timeouts after many tests. (Worded to avoid
                 the PC-side "ERROR" fail-fast keyword: recovery is benign.) */
              if (uartErrorPending > 0)
              {
                  UART_SendFormatted("UART RECOVERED: %lu faults\r\n", uartErrorPending);
                  uartErrorPending = 0;
              }
              /* Check for limit switch first */
              if (limitSwitchPressed)
              {
                  limitSwitchPressed = 0;
                  UART_Send("LIMIT SWITCH PRESSED SUCCESSFULLY\r\n");
              }
              /* Then check for normal commands - get one from execution queue */
              else
              {
                  status = osMessageQueueGet(execCommandQueueHandle, &execCmd, NULL, 0);

                  if (status == osOK)
                  {
                      /* A fresh command consumes any stale abort request left
                         by an ABORT that found the system idle (no worker was
                         around to observe and clear it). */
                      abortRequested = 0;
                      /* Debug: Got execution command */
                      // char dbg[32];
                      // snprintf(dbg, sizeof(dbg), "debug18: Exec start=%d\r\n", execCmd.start);
                      // UART_Send(dbg);

                      switch (execCmd.start)
                      {
                          case 1:
                              /* Debug: Starting sensor measurement */
                              // UART_Send("debug9: Sensor Start\r\n");
                              if (execCmd.sensor == SENSOR_AS7341)
                              {
                                  /* Store parameters for the sensor task */
                                  // The task will read from a global or use the execCmd
                                  uartCmdTemp.sensor = execCmd.sensor;
                                  uartCmdTemp.duration = execCmd.duration;
                                  uartCmdTemp.incubation = execCmd.incubation;
                                  uartCmdTemp.start = execCmd.start;

                                  systemState = STATE_AS7341_MEASURE;
                                  osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
                              }
                              else if (execCmd.sensor == SENSOR_LTR390)
                              {
                                  uartCmdTemp.sensor = execCmd.sensor;
                                  uartCmdTemp.duration = execCmd.duration;
                                  uartCmdTemp.incubation = execCmd.incubation;
                                  uartCmdTemp.start = execCmd.start;

                                  systemState = STATE_LTR390_MEASURE;
                                  osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
                              }
                              break;

                          case 2:
                              /* Debug: Stepper home start */
                              // UART_Send("debug10: Stepper Home\r\n");
                              systemState = STATE_STEPPER_HOME;
                              osThreadSetPriority(Stepper_HomeHandle, osPriorityNormal);
                              break;

                          case 3:
                              /* Debug: White LED align */
                              // UART_Send("debug11: White LED Align\r\n");
                              systemState = STATE_WHITE_LED_ALIGN;
                              osThreadSetPriority(White_Led_AlignHandle, osPriorityNormal);
                              break;

                          case 4:
                              /* Debug: UV LED align */
                              // UART_Send("debug12: UV LED Align\r\n");
                              systemState = STATE_UV_LED_ALIGN;
                              osThreadSetPriority(UV_Led_AlignHandle, osPriorityNormal);
                              break;

                          case 5:
                              /* Debug: Aspirate start */
                              // UART_Send("debug13: Aspirate\r\n");
                              systemState = STATE_ASPIRATE_SAMPLE;
                              osThreadSetPriority(AspirateHandle, osPriorityNormal);
                              break;

                          case 6:
                              /* Debug: Clean flow cell */
                              // UART_Send("debug14: Clean\r\n");
                              systemState = STATE_FLOW_CELL_CLEAN;
                              osThreadSetPriority(Clean_Flow_CellHandle, osPriorityNormal);
                              break;

                          default:
                              /* Debug: Unknown command */
                              // UART_Send("debug15: Unknown Command\r\n");
                              break;
                      }
                  }
              }
              break;

          /* Busy states - tasks handle these */
          case STATE_WHITE_LED_ALIGN:
          case STATE_UV_LED_ALIGN:
          case STATE_STEPPER_HOME:
          case STATE_ASPIRATE_SAMPLE:
          case STATE_AS7341_MEASURE:
          case STATE_LTR390_MEASURE:
          case STATE_FLOW_CELL_CLEAN:
              /* Do nothing - tasks own these states */
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

    for(;;)
    {
        uint8_t currentState =
            HAL_GPIO_ReadPin(Limit_SW_GPIO_Port, Limit_SW_Pin);

        /* Detect NEW physical press */
        if (currentState && !lastState)
        {
            /*
             * Accept only if system was IDLE
             * when the switch was pressed.
             */
            uint8_t acceptPress = (systemState == STATE_IDLE);

            /* Debounce */
            osDelay(50);

            /* Verify switch is still pressed */
            currentState =
                HAL_GPIO_ReadPin(Limit_SW_GPIO_Port, Limit_SW_Pin);

            /*
             * Successful limit switch press
             */
            if (currentState && acceptPress)
            {
                limitSwitchPressed = 1;

                UART_Send("LIMIT SWITCH PRESS SUCCESSFUL\r\n");
            }
        }

        /* Remember physical switch state */
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
	          UART_Send("FLOW CELL CLEAN START\r\n");

	          /* Try to acquire mutex with 100 tick timeout */
	          if (osMutexAcquire(pumpMutexHandle, MUTEX_TIMEOUT_TICKS) == osOK)
	          {
	              PUMP_Move(
	                  PUMP_FORWARD,
	                  2000,
	                  80
	              );

              PUMP_Stop();

              osMutexRelease(pumpMutexHandle);

              if (abortRequested)
              {
                  UART_Send("FLOW CELL CLEAN ABORTED\r\n");
              }
              else
              {
                  UART_Send("FLOW CELL CLEAN COMPLETE\r\n");
              }
          }
          else
          {
              UART_Send("CLEAN FAILED: PUMP BUSY\r\n");
          }

          /* ALWAYS reset state and clear command */
          uartCmdTemp.start = 0;
          uartCmdTemp.sensor = SENSOR_NONE;
          uartCmdTemp.duration = 0;
          uartCmdTemp.incubation = 0;

          /* Never clobber a newer operation that may have started */
          if (systemState == STATE_FLOW_CELL_CLEAN || systemState == STATE_IDLE)
          {
              systemState = STATE_IDLE;
          }
          abortRequested = 0;

	          osThreadSetPriority(
	              Clean_Flow_CellHandle,
	              osPriorityNormal
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

          UART_Send("PUMP INITIALIZATION START\r\n");

          /* Try to acquire mutex with 100 tick timeout */
          if (osMutexAcquire(pumpMutexHandle, MUTEX_TIMEOUT_TICKS) == osOK)
          {
              PUMP_Move(
                  PUMP_FORWARD,
                  1000,
                  80
              );

              PUMP_Stop();

              osMutexRelease(pumpMutexHandle);

              UART_Send("PUMP INITIALIZATION COMPLETE\r\n");
          }
          else
          {
              UART_Send("PUMP INITIALIZATION FAILED: PUMP BUSY\r\n");
          }
      }

      osDelay(10);
  }
  /* USER CODE END pump_initialize */
}

/* USER CODE BEGIN Header_reset */
/**
* @brief Function implementing the Reset thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_reset */
void reset(void *argument)
{
  /* USER CODE BEGIN reset */
    GPIO_PinState lastState = HAL_GPIO_ReadPin(Reset_GPIO_Port, Reset_Pin);
//	uint8_t lastState = 0;
  /* Infinite loop */
  for(;;)
  {
      GPIO_PinState currentState =
          HAL_GPIO_ReadPin(Reset_GPIO_Port, Reset_Pin);

      if ((currentState == GPIO_PIN_SET) &&
          (lastState == GPIO_PIN_RESET))
      {
          UART_Send("SOFTWARE RESET\r\n");

          osDelay(100);

          NVIC_SystemReset();
      }

      lastState = currentState;

      osDelay(10);
  }

  /* USER CODE END reset */
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
