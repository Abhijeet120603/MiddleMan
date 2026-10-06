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
    uint32_t delay;      /* Delay between readings in seconds */
    uint8_t start;      /* 1=sensor, 2=stepper, 3=white, 4=uv, 5=aspirate, 6=clean */
} UART_ExecutionCommand_t;

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

#define CANCEL_POLL_MS 25

#define TX_QUEUE_WAIT_TICKS 20

#define RX_BUFFER_SIZE 64

/* Add this for mutex timeouts */
#define MUTEX_TIMEOUT_MS    1000
#define MUTEX_TIMEOUT_TICKS (MUTEX_TIMEOUT_MS * configTICK_RATE_HZ / 1000)

#define BUSY_GRACE_MS 250
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
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* Definitions for AS7341_Send */
osThreadId_t AS7341_SendHandle;
const osThreadAttr_t AS7341_Send_attributes = {
  .name = "AS7341_Send",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Send_LTR390 */
osThreadId_t Send_LTR390Handle;
const osThreadAttr_t Send_LTR390_attributes = {
  .name = "Send_LTR390",
  .stack_size = 256 * 4,
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
/* Definitions for Cancel_Sensor_R */
osThreadId_t Cancel_Sensor_RHandle;
const osThreadAttr_t Cancel_Sensor_R_attributes = {
  .name = "Cancel_Sensor_R",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for pumpMutex */
osMutexId_t pumpMutexHandle;
const osMutexAttr_t pumpMutex_attributes = {
  .name = "pumpMutex"
};
/* Definitions for uart1Mutex */
//osMutexId_t uart1MutexHandle;
//const osMutexAttr_t uart1Mutex_attributes = {
//  .name = "uart1Mutex"
//};
/* Definitions for waitForIdleMutex */
osMutexId_t waitForIdleMutexHandle;
const osMutexAttr_t waitForIdleMutex_attributes = {
  .name = "waitForIdleMutex"
};
/* Definitions for messageI2C1_Lock */
osSemaphoreId_t messageI2C1_LockHandle;
const osSemaphoreAttr_t messageI2C1_Lock_attributes = {
  .name = "messageI2C1_Lock"
};
/* USER CODE BEGIN PV */

/* UART TX Queue */
osMessageQueueId_t uartTxQueueHandle;

const osMessageQueueAttr_t uartTxQueue_attributes = {
    .name = "uartTxQueue"
};

/* UART RX Queue */
osMessageQueueId_t uartRxQueueHandle;

const osMessageQueueAttr_t uartRxQueue_attributes = {
    .name = "uartRxQueue"
};


Adafruit_AS7341_t as7341;
uint16_t spectral[10];

volatile uint32_t uv340 = 0;

uint8_t rxByte;
volatile char rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;

uint8_t AS7341_Present = 0;

volatile SystemState_t systemState = STATE_IDLE;
volatile uint8_t limitSwitchPressed = 0;

volatile uint8_t initializePumpRequested = 0;

volatile uint8_t cancelSensorReading = 0;

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

volatile UART_ExecutionCommand_t uartCmdTemp = {0};

volatile uint32_t uartErrorCode = 0;

volatile uint8_t measurementComplete = 0;  // Flag to indicate measurement is done

volatile uint32_t uartRxQueueOverflow = 0;
volatile uint8_t  uartRxQueueOverflowFlag = 0;   /* set by ISR, cleared by dispatcher */

/* ===== UART DMA RX ===== */
#define UART_RX_DMA_SIZE 128
static uint8_t uartRxDMA[UART_RX_DMA_SIZE];

/* Persistent line assembler — survives across RxEvent callbacks */
static char     rxLineBuf[RX_COMMAND_SIZE];
static uint16_t rxLineIdx = 0;

volatile uint32_t uartRxIdleEvents   = 0;
volatile uint32_t uartRxHalfEvents   = 0;
volatile uint32_t uartRxDmaErrors    = 0;

/* ===== UART DMA TX ===== */
static UART_TxMessage_t uartTxCurrent;      /* must outlive the DMA */
static osSemaphoreId_t  uartTxDoneSemHandle;
static const osSemaphoreAttr_t uartTxDoneSem_attributes = {
    .name = "uartTxDoneSem"
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
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
void Cancel_Read_Sensor(void *argument);

/* USER CODE BEGIN PFP */
void ControlWhiteLED(uint8_t state);

void UART_Send(const char *message);
void UART_SendFormatted(const char *format, ...);
void ProcessUARTCommand(const char *command);

void WaitForIdleAndClean(void);

uint8_t WaitUntilOrAbort(uint32_t targetTick);
uint8_t WaitMsOrAbort(uint32_t ms);
void FinishAbortedRun(void);
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
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  PUMP_Init(&htim3);

  __HAL_UART_CLEAR_OREFLAG(&huart1);
  __HAL_UART_CLEAR_FEFLAG(&huart1);
  __HAL_UART_CLEAR_NEFLAG(&huart1);
  __HAL_UART_CLEAR_PEFLAG(&huart1);
  (void)huart1.Instance->DR;      /* read DR to clear RXNE if set */
  (void)huart1.Instance->SR;

//  ControlWhiteLED(LED_ON);   // WHITE LED ALWAYS ON

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of pumpMutex */
  pumpMutexHandle = osMutexNew(&pumpMutex_attributes);

  /* creation of uart1Mutex */
//  uart1MutexHandle = osMutexNew(&uart1Mutex_attributes);

  /* creation of waitForIdleMutex */
  waitForIdleMutexHandle = osMutexNew(&waitForIdleMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of messageI2C1_Lock */
  messageI2C1_LockHandle = osSemaphoreNew(1, 1, &messageI2C1_Lock_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  uartTxDoneSemHandle = osSemaphoreNew(1, 0, &uartTxDoneSem_attributes);
  /* count=1, initial=0 → starts "empty" */

  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

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


//  HAL_UART_Receive_IT(&huart1, &rxByte, 1);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uartRxDMA, UART_RX_DMA_SIZE);
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);   /* suppress HT events */


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

  /* creation of Cancel_Sensor_R */
  Cancel_Sensor_RHandle = osThreadNew(Cancel_Read_Sensor, NULL, &Cancel_Sensor_R_attributes);

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
//  HAL_UART_Receive_IT(&huart1, &rxByte, 1);
  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

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
  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Peltier2_Pin|Buzzer_Pin|Peltier1_Pin|DS18B20_Pin
                          |White_LED_Pin|UV_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Servo_Pin */
  GPIO_InitStruct.Pin = Servo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Servo_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : STEP_Pin EN_Pin DIR_Pin */
  GPIO_InitStruct.Pin = STEP_Pin|EN_Pin|DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Cancel_Sensor_Reading_Task_Pin Reset_Pin */
  GPIO_InitStruct.Pin = Cancel_Sensor_Reading_Task_Pin|Reset_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
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

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    static char cmdBuffer[RX_COMMAND_SIZE];
//    static uint8_t cmdIndex = 0;
//    osStatus_t status;
//
//    if (huart->Instance == USART1)
//    {
//        if (rxByte == '\r' || rxByte == '\n')
//        {
//            if (cmdIndex > 0)
//            {
//                cmdBuffer[cmdIndex] = '\0';
//
//                /* Trim trailing newline if present */
//                if (cmdBuffer[cmdIndex - 1] == '\n')
//                {
//                    cmdBuffer[cmdIndex - 1] = '\0';
//                    cmdIndex--;
//                }
//
//                if (cmdIndex > 0)
//                {
//                    UART_CommandMessage_t msg;
//                    strncpy(msg.command, cmdBuffer, RX_COMMAND_SIZE - 1);
//                    msg.command[RX_COMMAND_SIZE - 1] = '\0';
//
//                    /* Put command into RX queue - non-blocking from ISR */
//                    status = osMessageQueuePut(uartRxQueueHandle, &msg, 0, 0);
//
//                    if (status != osOK)
//                    {
//                        /* RX queue full - increment overflow counter */
//                        uartRxQueueOverflow++;
//                        uartRxQueueOverflowFlag = 1;
//
//                        /* Debug: Queue full */
//                        // UART_Send("debug0: RX Queue Full\r\n");
//                    }
//                    else
//                    {
//                        /* Debug: Command queued successfully */
//                        // UART_Send("debug1: RX Command Queued\r\n");
//                    }
//                }
//
//                cmdIndex = 0;
//            }
//        }
//        else
//        {
//            if (cmdIndex < RX_COMMAND_SIZE - 1)
//            {
//                cmdBuffer[cmdIndex++] = rxByte;
//            }
//            else
//            {
//                /* Buffer overflow - reset */
//                cmdIndex = 0;
//                memset(cmdBuffer, 0, sizeof(cmdBuffer));
//
//                /* Debug: Buffer overflow */
//                // UART_Send("debug2: RX Buffer Overflow\r\n");
//            }
//        }
//
//        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rxByte, 1);
//    }
//}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    static char cmdBuffer[RX_COMMAND_SIZE];
//    static uint8_t cmdIndex = 0;
//    osStatus_t status;
//
//    if (huart->Instance == USART1)
//    {
//        uint8_t b = rxByte;
//
//        if (b == '\r' || b == '\n')
//        {
//            if (cmdIndex > 0)
//            {
//                cmdBuffer[cmdIndex] = '\0';
//
//                /* Trim trailing CR if present */
//                if (cmdBuffer[cmdIndex - 1] == '\r')
//                {
//                    cmdBuffer[cmdIndex - 1] = '\0';
//                    cmdIndex--;
//                }
//
//                if (cmdIndex > 0)
//                {
//                    UART_CommandMessage_t msg;
//                    strncpy(msg.command, cmdBuffer, RX_COMMAND_SIZE - 1);
//                    msg.command[RX_COMMAND_SIZE - 1] = '\0';
//
//                    status = osMessageQueuePut(uartRxQueueHandle, &msg, 0, 0);
//                    if (status != osOK)
//                    {
//                        uartRxQueueOverflow++;
//                        uartRxQueueOverflowFlag = 1;
//                    }
//                }
//
//                cmdIndex = 0;
//            }
//        }
//        else if (b >= 0x20 && b <= 0x7E)   /* printable ASCII only */
//        {
//            if (cmdIndex < RX_COMMAND_SIZE - 1)
//            {
//                cmdBuffer[cmdIndex++] = (char)b;
//            }
//            else
//            {
//                cmdIndex = 0;
//                memset(cmdBuffer, 0, sizeof(cmdBuffer));
//            }
//        }
//        /* else: silently drop 0x00, 0xFF, break bytes, etc. */
//
//        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
//    }
//}



void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance != USART1) return;

    uartRxIdleEvents++;

    for (uint16_t i = 0; i < Size; i++)
    {
        char b = (char)uartRxDMA[i];

        if (b == '\r' || b == '\n')
        {
            if (rxLineIdx > 0)
            {
                rxLineBuf[rxLineIdx] = '\0';

                UART_CommandMessage_t msg;
                strncpy(msg.command, rxLineBuf, RX_COMMAND_SIZE - 1);
                msg.command[RX_COMMAND_SIZE - 1] = '\0';

                if (osMessageQueuePut(uartRxQueueHandle, &msg, 0, 0) != osOK)
                {
                    uartRxQueueOverflow++;
                    uartRxQueueOverflowFlag = 1;
                }
                rxLineIdx = 0;
            }
        }
        else if (b >= 0x20 && b <= 0x7E)
        {
            if (rxLineIdx < RX_COMMAND_SIZE - 1)
                rxLineBuf[rxLineIdx++] = b;
            else
                rxLineIdx = 0;   /* overflow → drop line */
        }
    }

    /* Re-arm RX DMA */
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uartRxDMA, UART_RX_DMA_SIZE) != HAL_OK)
    {
        uartRxDmaErrors++;
    }
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uartErrorCode = HAL_UART_GetError(huart);

        HAL_UART_AbortReceive(huart);

        if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uartRxDMA, UART_RX_DMA_SIZE) != HAL_OK)
        {
            uartRxDmaErrors++;
        }
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
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


uint8_t WaitUntilOrAbort(uint32_t targetTick)
{
    while ((int32_t)(targetTick - osKernelGetTickCount()) > 0)
    {
        uint32_t remaining;

        if (cancelSensorReading)
        {
            return 1;
        }

        remaining = targetTick - osKernelGetTickCount();
        osDelay((remaining > CANCEL_POLL_MS) ? CANCEL_POLL_MS : remaining);
    }

    return cancelSensorReading ? 1 : 0;
}


uint8_t WaitMsOrAbort(uint32_t ms)
{
    return WaitUntilOrAbort(osKernelGetTickCount() + ms);
}


void FinishAbortedRun(void)
{
    ControlWhiteLED(LED_OFF);
    HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

    uartCmdTemp.start = 0;
    uartCmdTemp.sensor = SENSOR_NONE;

    cancelSensorReading = 0;
    measurementComplete = 1;
    systemState = STATE_IDLE;

    UART_Send("ABORT COMPLETE\r\n");


    osThreadYield();
}

void ProcessUARTCommand(const char *command)
{
    if (strcmp(command, "PING") == 0 || strcmp(command, "ping") == 0)
    {

        UART_Send("PONG\r\n");
        return;
    }
    else if (strcmp(command, "HELP") == 0 || strcmp(command, "help") == 0)
    {
        UART_Send(
            "Available commands:\r\n"
            "PING - Test connection\r\n"
            "STEPPER_HOME - Home the stepper motor\r\n"
            "WHITE_LED_ALIGN - Start white LED alignment\r\n"
            "UV_LED_ALIGN - Start UV LED alignment\r\n"
            "ASPIRATE - Run aspirate sample sequence\r\n"
            "AS7341,<duration>,<incubation>,<delay> - AS7341 sensor measurement\r\n"
            "LTR390,<duration>,<incubation>,<delay> - LTR390 sensor measurement\r\n"
            "CLEAN - Run flow cell cleaning\r\n"
            "CLEAN_WAIT - Wait for IDLE then run flow cell cleaning\r\n"
        );
        return;
    }


    if (strcmp(command, "CLEAN_WAIT") == 0 || strcmp(command, "clean_wait") == 0)
    {
        WaitForIdleAndClean();
        return;
    }

   if (strcmp(command, "STATUS") == 0 || strcmp(command, "status") == 0)
    {
        UART_SendFormatted("STATUS,%u,%u,%u\r\n",
                           (unsigned)systemState,
                           (unsigned)cancelSensorReading,
                           (unsigned)uartRxQueueOverflow);
        return;
    }

   if (strcmp(command, "ABORT") == 0 || strcmp(command, "abort") == 0)
    {
        cancelSensorReading = 1;
        UART_Send("ABORT ACKNOWLEDGED\r\n");

        if (systemState != STATE_AS7341_MEASURE &&
            systemState != STATE_LTR390_MEASURE)
        {
            cancelSensorReading = 0;
            UART_Send("ABORT COMPLETE\r\n");
        }

        return;
    }


    if (systemState != STATE_IDLE)
    {
        uint32_t graceEnd = osKernelGetTickCount() + BUSY_GRACE_MS;

        while (systemState != STATE_IDLE &&
               (int32_t)(graceEnd - osKernelGetTickCount()) > 0)
        {
            osDelay(1);
        }

        if (systemState != STATE_IDLE)
        {
            UART_Send("BUSY - Command rejected\r\n");
            return;
        }
    }


    UART_ExecutionCommand_t execCmd = {0};
    int start = 0;

    cancelSensorReading = 0;

    if (strcmp(command, "STEPPER_HOME") == 0 || strcmp(command, "stepper_home") == 0)
    {
        start = 2;
    }
    else if (strcmp(command, "WHITE_LED_ALIGN") == 0 || strcmp(command, "white_led_align") == 0)
    {
        start = 3;
    }
    else if (strcmp(command, "UV_LED_ALIGN") == 0 || strcmp(command, "uv_led_align") == 0)
    {
        start = 4;
    }
    else if (strcmp(command, "ASPIRATE") == 0 || strcmp(command, "aspirate") == 0 ||
             strcmp(command, "ASPIRATE_SAMPLE") == 0)
    {
        start = 5;
    }
    else if (strcmp(command, "CLEAN") == 0 || strcmp(command, "clean") == 0)
    {
        start = 6;
    }
    else
    {
        /* Parse sensor commands: SENSOR,duration,incubation,delay */
        char sensor[20];
        int sec = 0;
        int incubate = 0;
        int interval = 1;

        int parsed = sscanf(command, "%[^,],%d,%d,%d", sensor, &sec, &incubate, &interval);

        if (parsed >= 2)
        {
            if (interval <= 1)
            {
                interval = 1; /* Default to 1-second delay if 0 or 1 passed */
            }

            if (strcmp(sensor, "AS7341") == 0)
            {
                execCmd.sensor = SENSOR_AS7341;
                execCmd.duration = sec;
                execCmd.incubation = incubate;
                execCmd.delay = interval;
                start = 1;
            }
            else if (strcmp(sensor, "LTR390") == 0)
            {
                execCmd.sensor = SENSOR_LTR390;
                execCmd.duration = sec;
                execCmd.incubation = incubate;
                execCmd.delay = interval;
                start = 1;
            }
            else
            {
//                UART_Send("UNKNOWN COMMAND\r\n");
            	UART_SendFormatted("UNKNOWN COMMAND: '%s'\r\n", command);
                return;
            }
        }
        else
        {
//            UART_Send("UNKNOWN COMMAND\r\n");
        	UART_SendFormatted("UNKNOWN COMMAND: '%s'\r\n", command);
            return;
        }
    }

    /* Store for thread execution */
    uartCmdTemp.sensor = execCmd.sensor;
    uartCmdTemp.duration = execCmd.duration;
    uartCmdTemp.incubation = execCmd.incubation;
    uartCmdTemp.delay = execCmd.delay;
    uartCmdTemp.start = start;

    /* Set state and trigger the appropriate task */
    switch (start)
    {
        case 1:

        	measurementComplete = 0;

            if (execCmd.sensor == SENSOR_AS7341)
            {
                systemState = STATE_AS7341_MEASURE;
                osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);
            }
            else if (execCmd.sensor == SENSOR_LTR390)
            {
                systemState = STATE_LTR390_MEASURE;
                osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
            }
            UART_Send("SENSOR MEASUREMENT START\r\n");
            break;
        case 2:
            systemState = STATE_STEPPER_HOME;
            osThreadSetPriority(Stepper_HomeHandle, osPriorityNormal);
            UART_Send("STEPPER HOME START\r\n");
            break;
        case 3:
            systemState = STATE_WHITE_LED_ALIGN;
            osThreadSetPriority(White_Led_AlignHandle, osPriorityNormal);
            UART_Send("WHITE LED ALIGN START\r\n");
            break;
        case 4:
            systemState = STATE_UV_LED_ALIGN;
            osThreadSetPriority(UV_Led_AlignHandle, osPriorityNormal);
            UART_Send("UV LED ALIGN START\r\n");
            break;
        case 5:
            systemState = STATE_ASPIRATE_SAMPLE;
            osThreadSetPriority(AspirateHandle, osPriorityNormal);
            UART_Send("ASPIRATE SAMPLE START\r\n");
            break;
        case 6:
            systemState = STATE_FLOW_CELL_CLEAN;
            osThreadSetPriority(Clean_Flow_CellHandle, osPriorityNormal);
            UART_Send("CLEAN COMMAND RECEIVED\r\n");
            break;
        default:
//            UART_Send("UNKNOWN COMMAND\r\n");
        	UART_SendFormatted("UNKNOWN COMMAND: '%s'\r\n", command);
            return;
    }

    UART_Send("COMMAND ACCEPTED\r\n");
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

     status = osMessageQueuePut(uartTxQueueHandle, &msg, 0, TX_QUEUE_WAIT_TICKS);

    if (status != osOK)
    {
        uartTxQueueFull++;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        osSemaphoreRelease(uartTxDoneSemHandle);   /* ISR-safe in CMSIS-OS v2 */
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

void WaitForIdleAndClean(void)
{
    // Wait for system to become IDLE
    uint32_t timeout = osKernelGetTickCount() + 5000; // 5 second timeout

    while (systemState != STATE_IDLE && osKernelGetTickCount() < timeout)
    {
        osDelay(10);
    }

    if (systemState == STATE_IDLE)
    {
        UART_Send("READY_FOR_CLEAN\r\n");

        uartCmdTemp.start = 6;
        systemState = STATE_FLOW_CELL_CLEAN;

        osThreadSetPriority(Clean_Flow_CellHandle, osPriorityNormal);

        UART_Send("CLEAN COMMAND RECEIVED\r\n");
    }
    else
    {
        UART_Send("TIMEOUT_WAITING_FOR_IDLE\r\n");
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

          Adafruit_AS7341_setATIME(&as7341, 50);
          Adafruit_AS7341_setASTEP(&as7341, 999);
          Adafruit_AS7341_setGain(&as7341, AS7341_GAIN_16X);

          // Ensure LED is off initially
          ControlWhiteLED(LED_OFF);

          // 1. Incubation phase delay (abortable - ABORT must stop the run even
          //    while the board is incubating)
          if (uartCmdTemp.incubation > 0 &&
              WaitMsOrAbort(uartCmdTemp.incubation * 1000))
          {
              FinishAbortedRun();
              continue;
          }

          // 2. Prepare timing and total reading count parameters
          uint32_t totalDuration = uartCmdTemp.duration;
          uint32_t readingInterval = (uartCmdTemp.delay > 0) ? uartCmdTemp.delay : 1;
          uint32_t totalReadings = totalDuration / readingInterval;

          uint32_t startTime = osKernelGetTickCount();
          uint8_t aborted = 0;

          for (uint32_t readingIdx = 1; readingIdx <= totalReadings; readingIdx++)
          {
              uint32_t targetReadingTimeSec = readingIdx * readingInterval;

              // Turn ON LED 1 second before reading time
              uint32_t ledOnTimeMs = startTime + ((targetReadingTimeSec - 1) * 1000);
              if (WaitUntilOrAbort(ledOnTimeMs))
              {
                  aborted = 1;
                  break;
              }
              ControlWhiteLED(LED_ON);

              // Wait remaining 1s until exact reading timestamp
              uint32_t sampleTimeMs = startTime + (targetReadingTimeSec * 1000);
              if (WaitUntilOrAbort(sampleTimeMs))
              {
                  aborted = 1;
                  break;
              }

              // Perform sensor reading
              if(Adafruit_AS7341_take10ChannelReadings(&as7341, spectral))
              {
                  char tx[100];
                  snprintf(tx,
                           sizeof(tx),
                           "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
                           targetReadingTimeSec,
                           spectral[0], spectral[1], spectral[2], spectral[3],
                           spectral[4], spectral[5], spectral[6], spectral[7],
                           spectral[8], spectral[9]);

                  UART_Send(tx);
              }

              // Turn OFF LED immediately after reading
              ControlWhiteLED(LED_OFF);
          }

          if (aborted)
          {
              // Host pressed Back / sent ABORT: release the board immediately
              // (idle + LEDs off + ABORT COMPLETE) so the next command runs.
              FinishAbortedRun();
              continue;
          }

          // Reset command variables
          uartCmdTemp.start = 0;
          uartCmdTemp.sensor = SENSOR_NONE;
          measurementComplete = 1;
          systemState = STATE_IDLE;
          osThreadSetPriority(AS7341_SendHandle, osPriorityNormal);

          UART_Send("MEASUREMENT_COMPLETE\r\n");

          osThreadYield();
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

      if (systemState == STATE_LTR390_MEASURE)
      {
          if (HAL_I2C_IsDeviceReady(&hi2c3,
                                    0x53 << 1,
                                    2,
                                    MUTEX_TIMEOUT_TICKS) != HAL_OK)
          {
              UART_Send("LTR390 NOT FOUND\r\n");
              systemState = STATE_IDLE;
              osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);
              continue;
          }

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

          osDelay(450);

          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

          uint8_t aborted = 0;

           if (uartCmdTemp.incubation > 0)
          {
              uint32_t incubateEnd = osKernelGetTickCount()
                                     + (uartCmdTemp.incubation * 1000);

              if (WaitUntilOrAbort(incubateEnd))
              {
                  FinishAbortedRun();
                  continue;
              }
          }

          /* UV LED on for the entire measurement window */
          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_SET);

          uint32_t totalDuration   = uartCmdTemp.duration;
          uint32_t readingInterval = (uartCmdTemp.delay > 0) ? uartCmdTemp.delay : 1;
          uint32_t totalReadings   = totalDuration / readingInterval;

          uint32_t startTime = osKernelGetTickCount();

          for (uint32_t readingIdx = 1; readingIdx <= totalReadings; readingIdx++)
          {
              uint32_t targetReadingTimeSec = readingIdx * readingInterval;
              uint32_t sampleTimeMs = startTime + (targetReadingTimeSec * 1000);

              if (WaitUntilOrAbort(sampleTimeMs))
              {
                  aborted = 1;
                  break;
              }

              if (LTR390_NewDataAvailable(&ltr))
              {
                  uv340 = LTR390_ReadUVS(&ltr);
              }

              {
                  char tx[30];
                  snprintf(tx, sizeof(tx), "%lu,%lu\r\n",
                           targetReadingTimeSec, uv340);
                  UART_Send(tx);
              }
          }

          HAL_GPIO_WritePin(UV_LED_GPIO_Port, UV_LED_Pin, GPIO_PIN_RESET);

          if (aborted)
          {
              FinishAbortedRun();
              continue;
          }

          uartCmdTemp.start  = 0;
          uartCmdTemp.sensor = SENSOR_NONE;

          measurementComplete = 1;
          systemState = STATE_IDLE;
          osThreadSetPriority(Send_LTR390Handle, osPriorityNormal);

          UART_Send("MEASUREMENT_COMPLETE\r\n");

          osThreadYield();
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
         if (osMessageQueueGet(uartTxQueueHandle, &msg, NULL, osWaitForever) != osOK)
             continue;

         /* Copy into the static buffer that DMA will read from */
         uartTxCurrent = msg;

         if (HAL_UART_Transmit_DMA(&huart1,
                                   uartTxCurrent.data,
                                   uartTxCurrent.length) != HAL_OK)
         {
             uartTxQueueFull++;   /* or a dedicated error counter */
             continue;
         }

         /* Wait for TxCplt callback to release us */
         osSemaphoreAcquire(uartTxDoneSemHandle, osWaitForever);
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

            // Call your stepper home function
            Stepper_Home();


            UART_Send("STEPPER HOME COMPLETE\r\n");

            // Return to IDLE
            systemState = STATE_IDLE;
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

        // Perform alignment

        Stepper_White_LED_Align();


        UART_Send("WHITE LED ALIGN COMPLETE\r\n");

        // Return to IDLE
        systemState = STATE_IDLE;
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

            UART_Send("UV LED ALIGN COMPLETE\r\n");

            // Return to IDLE
            systemState = STATE_IDLE;
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

            /* Acquire pump mutex with timeout */
            if (osMutexAcquire(pumpMutexHandle, MUTEX_TIMEOUT_TICKS) == osOK)
            {
                PUMP_Move(
                    PUMP_FORWARD,
                    400, //500
                    75
                );

                HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
                osDelay(100);
                HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);

                osDelay(2000);

                PUMP_Move(
                    PUMP_FORWARD,
                    360, //300
                    80
                );

                PUMP_Stop();

                osMutexRelease(pumpMutexHandle);

                UART_Send("ASPIRATE SAMPLE COMPLETE\r\n");
            }
            else
            {
                UART_Send("ASPIRATE FAILED: PUMP BUSY\r\n");
            }

            uartCmdTemp.start = 0;
            uartCmdTemp.sensor = SENSOR_NONE;
            uartCmdTemp.duration = 0;
            uartCmdTemp.incubation = 0;

            systemState = STATE_IDLE;

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
    osStatus_t status;
  /* Infinite loop */
  for(;;)
  {
      /* Report any UART errors */
      if (uartErrorCode != 0)
      {
          uint32_t err = uartErrorCode;
          uartErrorCode = 0;

          UART_Send("UART ERROR: ");
          if (err & HAL_UART_ERROR_PE) UART_Send("PARITY ");
          if (err & HAL_UART_ERROR_NE) UART_Send("NOISE ");
          if (err & HAL_UART_ERROR_FE) UART_Send("FRAME ");
          if (err & HAL_UART_ERROR_ORE) UART_Send("OVERRUN ");
          UART_Send("\r\n");

          /* NOTE: do NOT touch HAL_UART_Receive_IT / AbortReceive here.
             HAL_UART_ErrorCallback already aborts and re-arms RX.
             Re-arming from here races the ISR and can leave RX disarmed
             (HAL_BUSY returned and ignored), which is exactly what caused
             the "measurement #4 got zero bytes" failure. */
      }

      /* Report RX queue overflow (latched by the ISR) */
      if (uartRxQueueOverflowFlag)
      {
          uartRxQueueOverflowFlag = 0;
          UART_SendFormatted("RX QUEUE FULL (total=%lu)\r\n",
                             (unsigned long)uartRxQueueOverflow);
      }

      /* Check for commands */
      status = osMessageQueueGet(uartRxQueueHandle, &cmdMsg, NULL, 0);
      if (status == osOK)
      {
          ProcessUARTCommand(cmdMsg.command);
      }

      /* State machine */
      switch (systemState)
      {
          case STATE_IDLE:
              if (limitSwitchPressed)
              {
                  limitSwitchPressed = 0;
                  UART_Send("LIMIT SWITCH PRESSED SUCCESSFULLY\r\n");
              }
              break;

          case STATE_WHITE_LED_ALIGN:
          case STATE_UV_LED_ALIGN:
          case STATE_STEPPER_HOME:
          case STATE_ASPIRATE_SAMPLE:
          case STATE_AS7341_MEASURE:
          case STATE_LTR390_MEASURE:
          case STATE_FLOW_CELL_CLEAN:
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

//	          /* Try to acquire mutex with 100 tick timeout */
//	          if (osMutexAcquire(pumpMutexHandle, MUTEX_TIMEOUT_TICKS) == osOK)
//	          {
	              PUMP_Move(
	                  PUMP_FORWARD,
	                  1500,
	                  80
	              );

	              PUMP_Stop();

//	              osMutexRelease(pumpMutexHandle);

	              UART_Send("FLOW CELL CLEAN COMPLETE\r\n");
//	          }
//	          else
//	          {
//	              UART_Send("CLEAN FAILED: PUMP BUSY\r\n");
//	          }

	          /* ALWAYS reset state and clear command */
	          uartCmdTemp.start = 0;
	          uartCmdTemp.sensor = SENSOR_NONE;
	          uartCmdTemp.duration = 0;
	          uartCmdTemp.incubation = 0;

	          systemState = STATE_IDLE;

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

/* USER CODE BEGIN Header_Cancel_Read_Sensor */
/**
* @brief Function implementing the Cancel_Sensor_R thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Cancel_Read_Sensor */
void Cancel_Read_Sensor(void *argument)
{
  /* USER CODE BEGIN Cancel_Read_Sensor */
//    uint8_t lastPinState = 0;
  /* Cancellation is driven by the ABORT command (see ProcessUARTCommand),
     which the sensor workers honour through cancelSensorReading. */
  /* Infinite loop */
  for(;;)
  {
      /* This loop used to spin with no delay and no body at all, which burnt
         the lowest-priority slot continuously. A tick of slack keeps the
         scheduler (and UART service) responsive. */
      osDelay(50);
//      uint8_t pinState = HAL_GPIO_ReadPin(
//          Cancel_Sensor_Reading_Task_GPIO_Port,
//          Cancel_Sensor_Reading_Task_Pin
//      );
//
//      /* Detect rising edge */
//      if (pinState == GPIO_PIN_SET && lastPinState == GPIO_PIN_RESET)
//      {
//          /* Only act if a sensor measurement is running */
//          if (systemState == STATE_AS7341_MEASURE ||
//              systemState == STATE_LTR390_MEASURE)
//          {
//              cancelSensorReading = 1;
//              UART_Send("CANCEL REQUEST RECEIVED\r\n");
//          }
//          else
//          {
//              UART_Send("CANCEL IGNORED: NO SENSOR READING ACTIVE\r\n");
//          }
//      }
//
//      lastPinState = pinState;
//      osDelay(20);   /* 20 ms poll — debounce naturally */
  }
  /* USER CODE END Cancel_Read_Sensor */
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
