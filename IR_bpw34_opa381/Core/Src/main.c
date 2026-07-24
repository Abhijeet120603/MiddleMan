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
#include <stdio.h>
#include <string.h>
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

UART_HandleTypeDef huart1;

/* Definitions for Read_Bpw34 */
osThreadId_t Read_Bpw34Handle;
const osThreadAttr_t Read_Bpw34_attributes = {
  .name = "Read_Bpw34",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Send_Bpw34 */
osThreadId_t Send_Bpw34Handle;
const osThreadAttr_t Send_Bpw34_attributes = {
  .name = "Send_Bpw34",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Command_Handler */
osThreadId_t Command_HandlerHandle;
const osThreadAttr_t Command_Handler_attributes = {
  .name = "Command_Handler",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Data_Holder */
osMessageQueueId_t Data_HolderHandle;
const osMessageQueueAttr_t Data_Holder_attributes = {
  .name = "Data_Holder"
};
/* USER CODE BEGIN PV */
static int16_t adc_value = 0;
char uart_buffer[32];

#define UART_RX_BUFFER_SIZE 32
uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
uint8_t rx_data[UART_RX_BUFFER_SIZE];
uint8_t rx_index = 0;
uint8_t command_received = 0;
uint8_t handshake_complete = 0;
uint32_t reading_duration = 0;  // in seconds
uint32_t reading_count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartCommandHandler(void *argument);

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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Data_Holder */
  Data_HolderHandle = osMessageQueueNew (16, sizeof(int16_t), &Data_Holder_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Read_Bpw34 */
  Read_Bpw34Handle = osThreadNew(StartDefaultTask, NULL, &Read_Bpw34_attributes);

  /* creation of Send_Bpw34 */
  Send_Bpw34Handle = osThreadNew(StartTask02, NULL, &Send_Bpw34_attributes);

  /* creation of Command_Handler */
  Command_HandlerHandle = osThreadNew(StartCommandHandler, NULL, &Command_Handler_attributes);

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

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  HAL_UART_Receive_IT(&huart1, rx_data, 1);
  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    // Store received character
    rx_buffer[rx_index++] = rx_data[0];

    // Check for newline or carriage return (end of command)
    if (rx_data[0] == '\n' || rx_data[0] == '\r')
    {
      rx_buffer[rx_index - 1] = '\0';  // Null terminate string
      command_received = 1;
      rx_index = 0;
    }
    else if (rx_index >= UART_RX_BUFFER_SIZE - 1)
    {
      // Buffer overflow, reset
      rx_index = 0;
    }

    // Start next reception
    HAL_UART_Receive_IT(&huart1, rx_data, 1);
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Optional: Handle UART transmit complete */
  if (huart->Instance == USART1)
  {
    /* Set a flag or do something when transmission is complete */
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	osStatus_t status;

  /* Infinite loop */
  for(;;)
  {
	  if (reading_duration > 0 && reading_count < reading_duration)
	      {
	        HAL_ADC_Start(&hadc1);

	        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
	        {
	          adc_value = (int16_t)HAL_ADC_GetValue(&hadc1);

	          status = osMessageQueuePut(
	                    Data_HolderHandle,
	                    &adc_value,
	                    0,
	                    0);

	          reading_count++;  // Increment reading count
	        }
	        HAL_ADC_Stop(&hadc1);

	        // Check if we've completed the requested duration
	        if (reading_count >= reading_duration)
	        {
	          char done_msg[] = "READING_COMPLETE\n";
	          HAL_UART_Transmit(&huart1, (uint8_t*)done_msg, strlen(done_msg), 100);
	          reading_duration = 0;  // Reset for next session
	        }
	      }

	  osDelay(1000);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Send_Bpw34 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
	int16_t received_value;
    osStatus_t status;
    uint8_t tx_buffer[32];
    int len;
  /* Infinite loop */
  for(;;)
  {
	  status = osMessageQueueGet(Data_HolderHandle, &received_value, NULL, osWaitForever);

	  if (status == osOK && reading_duration > 0){
			/* Option 1: Send as raw binary data (2 bytes) */
			// tx_buffer[0] = (received_value >> 8) & 0xFF;  // High byte
			// tx_buffer[1] = received_value & 0xFF;         // Low byte
			// HAL_UART_Transmit(&huart1, tx_buffer, 2, 100);

			/* Option 2: Send as formatted string (human readable) */
//			len = snprintf((char*)tx_buffer, sizeof(tx_buffer), "ADC: %d\r\n", received_value);
//			HAL_UART_Transmit(&huart1, tx_buffer, len, 100);

			/* Option 3: Send as CSV format for easier parsing */
			 len = snprintf((char*)tx_buffer, sizeof(tx_buffer), "%d\r\n", received_value);
			 HAL_UART_Transmit(&huart1, tx_buffer, len, 100);
	  }

  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartCommandHandler */
/**
* @brief Function implementing the Command_Handler thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommandHandler */
void StartCommandHandler(void *argument)
{
  /* USER CODE BEGIN StartCommandHandler */

	char cmd_buffer[32];
	uint32_t duration;
  /* Infinite loop */
  for(;;)
  {

	  if (command_received)
	      {
	        command_received = 0;
	        strcpy(cmd_buffer, (char*)rx_buffer);

	        // Check for "PING" command
	        if (strncmp(cmd_buffer, "PING", 4) == 0)
	        {
	          // Send PONG response
	          char response[] = "PONG\n";
	          HAL_UART_Transmit(&huart1, (uint8_t*)response, strlen(response), 100);
	          handshake_complete = 1;
	        }
	        // Check for duration command (format: "DURATION:10" for 10 seconds)
	        else if (strncmp(cmd_buffer, "DURATION:", 9) == 0)
	        {
	          // Parse the duration value
	          duration = atoi(cmd_buffer + 9);
	          if (duration > 0 && duration <= 3600)  // Max 1 hour
	          {
	            reading_duration = duration;
	            reading_count = 0;
	            // Acknowledge the duration command
	            char ack[32];
	            snprintf(ack, sizeof(ack), "ACK:%lu\n", duration);
	            HAL_UART_Transmit(&huart1, (uint8_t*)ack, strlen(ack), 100);
	          }
	          else
	          {
	            char error[] = "ERROR:Invalid duration\n";
	            HAL_UART_Transmit(&huart1, (uint8_t*)error, strlen(error), 100);
	          }
	        }
	        // Check for "START" command
	        else if (strncmp(cmd_buffer, "START", 5) == 0)
	        {
	          if (handshake_complete && reading_duration > 0)
	          {
	            char start_msg[] = "START_READING\n";
	            HAL_UART_Transmit(&huart1, (uint8_t*)start_msg, strlen(start_msg), 100);
	            reading_count = 0;
	          }
	          else
	          {
	            char error[] = "ERROR:Handshake not complete or duration not set\n";
	            HAL_UART_Transmit(&huart1, (uint8_t*)error, strlen(error), 100);
	          }
	        }
	        else if (strncmp(cmd_buffer, "STOP", 4) == 0)
	        {
	          reading_duration = 0;
	          char stop_msg[] = "STOPPED\n";
	          HAL_UART_Transmit(&huart1, (uint8_t*)stop_msg, strlen(stop_msg), 100);
	        }
	        else
	        {
	          char error[] = "ERROR:Unknown command\n";
	          HAL_UART_Transmit(&huart1, (uint8_t*)error, strlen(error), 100);
	        }
	      }
    osDelay(10);
  }
  /* USER CODE END StartCommandHandler */
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
