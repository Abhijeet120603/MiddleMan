///////* USER CODE BEGIN Header */
///////**
//////  ******************************************************************************
//////  * @file           : main.c
//////  * @brief          : Main program body
//////  ******************************************************************************
//////  * @attention
//////  *
//////  * Copyright (c) 2025 STMicroelectronics.
//////  * All rights reserved.
//////  *
//////  * This software is licensed under terms that can be found in the LICENSE file
//////  * in the root directory of this software component.
//////  * If no LICENSE file comes with this software, it is provided AS-IS.
//////  *
//////  ******************************************************************************
//////  */
///////* USER CODE END Header */
///////* Includes ------------------------------------------------------------------*/
//////#include "main.h"
//////#include "cmsis_os.h"
//////#include "crc.h"
//////#include "dma2d.h"
//////#include "i2c.h"
//////#include "ltdc.h"
//////#include "spi.h"
//////#include "tim.h"
//////#include "usart.h"
//////#include "usb_host.h"
//////#include "gpio.h"
//////#include "fmc.h"
//////
///////* Includes */
//////#include "stm32f4xx_hal.h"
//////#include "DIALOG.h"
//////#include "stm32f429i_discovery.h"
//////#include "stm32f429i_discovery_lcd.h"
//////
//////#include "main.h"
//////#include "stm32f4xx_hal.h"
//////#include "stm32f429i_discovery.h"
//////#include "stm32f429i_discovery_lcd.h"
///////* Private variables */
///////* Private includes ----------------------------------------------------------*/
///////* USER CODE BEGIN Includes */
//////#include "main.h"
//////#include "stm32f429i_discovery.h"
//////#include "stm32f429i_discovery_lcd.h"
//////#include "stm32f429i_discovery_sdram.h"
//////
///////* USER CODE END Includes */
//////
///////* Private typedef -----------------------------------------------------------*/
///////* USER CODE BEGIN PTD */
//////
///////* USER CODE END PTD */
//////
///////* Private define ------------------------------------------------------------*/
///////* USER CODE BEGIN PD */
//////
///////* USER CODE END PD */
//////
///////* Private macro -------------------------------------------------------------*/
///////* USER CODE BEGIN PM */
//////
///////* USER CODE END PM */
//////
///////* Private variables ---------------------------------------------------------*/
//////
///////* USER CODE BEGIN PV */
//////
///////* USER CODE END PV */
//////
///////* Private function prototypes -----------------------------------------------*/
//////void SystemClock_Config(void);
//////void MX_FREERTOS_Init(void);
///////* USER CODE BEGIN PFP */
//////
///////* USER CODE END PFP */
//////
///////* Private user code ---------------------------------------------------------*/
///////* USER CODE BEGIN 0 */
//////
///////* USER CODE END 0 */
//////
///////**
//////  * @brief  The application entry point.
//////  * @retval int
//////  */
//////int main(void)
//////{
//////  HAL_Init();
//////  SystemClock_Config();
//////
//////  // Initialize critical peripherals
//////  MX_GPIO_Init();
//////  MX_FMC_Init();
//////  MX_LTDC_Init();
//////
//////  // Enable backlight
//////  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
//////
//////  // Initialize SDRAM and LCD
//////  BSP_SDRAM_Init();
//////  BSP_LCD_Init();
//////  // After BSP_LCD_Init()
//////  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
//////  BSP_LCD_SelectLayer(0);
//////
//////  // Fill screen with solid red
//////  BSP_LCD_Clear(LCD_COLOR_RED);
//////  HAL_Delay(1000);
//////
//////  // Fill screen with solid green
//////  BSP_LCD_Clear(LCD_COLOR_GREEN);
//////  HAL_Delay(1000);
//////
//////  // Fill screen with solid blue
//////  BSP_LCD_Clear(LCD_COLOR_BLUE);
//////  HAL_Delay(1000);
//////
//////  // Draw test pattern
//////  for(int y = 0; y < BSP_LCD_GetYSize(); y++) {
//////      for(int x = 0; x < BSP_LCD_GetXSize(); x++) {
//////          uint16_t color = ((x/16 + y/16) % 2) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
//////          BSP_LCD_DrawPixel(x, y, color);
//////      }
//////  }
//////  HAL_Delay(100);
//////
//////  // Configure layer
//////  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
//////  BSP_LCD_SelectLayer(0);
//////
//////  // Display text
//////  BSP_LCD_Clear(LCD_COLOR_BLUE);
//////  BSP_LCD_SetFont(&Font24);
//////  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
//////  BSP_LCD_DisplayStringAt(0, LINE(5), (uint8_t *)"HELLO", CENTER_MODE);
//////
//////
//////}
//////
//////
///////**
//////  * @brief System Clock Configuration
//////  * @retval None
//////  */
//////void SystemClock_Config(void)
//////{
//////  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//////  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//////
//////  /** Configure the main internal regulator output voltage
//////  */
//////  __HAL_RCC_PWR_CLK_ENABLE();
//////  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
//////
//////  /** Initializes the RCC Oscillators according to the specified parameters
//////  * in the RCC_OscInitTypeDef structure.
//////  */
//////  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//////  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//////  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//////  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//////  RCC_OscInitStruct.PLL.PLLM = 4;
//////  RCC_OscInitStruct.PLL.PLLN = 72;
//////  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//////  RCC_OscInitStruct.PLL.PLLQ = 3;
//////  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//////  {
//////    Error_Handler();
//////  }
//////
//////  /** Initializes the CPU, AHB and APB buses clocks
//////  */
//////  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//////                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//////  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//////  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//////  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
//////  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
//////
//////  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
//////  {
//////    Error_Handler();
//////  }
//////}
//////
///////* USER CODE BEGIN 4 */
//////
///////* USER CODE END 4 */
//////
///////**
//////  * @brief  Period elapsed callback in non blocking mode
//////  * @note   This function is called  when TIM6 interrupt took place, inside
//////  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
//////  * a global variable "uwTick" used as application time base.
//////  * @param  htim : TIM handle
//////  * @retval None
//////  */
//////void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//////{
//////  /* USER CODE BEGIN Callback 0 */
//////
//////  /* USER CODE END Callback 0 */
//////  if (htim->Instance == TIM6)
//////  {
//////    HAL_IncTick();
//////  }
//////  /* USER CODE BEGIN Callback 1 */
//////
//////  /* USER CODE END Callback 1 */
//////}
//////
///////**
//////  * @brief  This function is executed in case of error occurrence.
//////  * @retval None
//////  */
//////void Error_Handler(void)
//////{
//////  /* USER CODE BEGIN Error_Handler_Debug */
//////  /* User can add his own implementation to report the HAL error return state */
//////  __disable_irq();
//////  while (1)
//////  {
//////  }
//////  /* USER CODE END Error_Handler_Debug */
//////}
//////#ifdef USE_FULL_ASSERT
///////**
//////  * @brief  Reports the name of the source file and the source line number
//////  *         where the assert_param error has occurred.
//////  * @param  file: pointer to the source file name
//////  * @param  line: assert_param error line source number
//////  * @retval None
//////  */
//////void assert_failed(uint8_t *file, uint32_t line)
//////{
//////  /* USER CODE BEGIN 6 */
//////  /* User can add his own implementation to report the file name and line number,
//////     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//////  /* USER CODE END 6 */
//////}
//////#endif /* USE_FULL_ASSERT */
////
////
////
////
/////* USER CODE BEGIN Header */
/////**
////  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body
////  ******************************************************************************
////  * @attention
////  *
////  * Copyright (c) 2025 STMicroelectronics.
////  * All rights reserved.
////  *
////  * This software is licensed under terms that can be found in the LICENSE file
////  * in the root directory of this software component.
////  * If no LICENSE file comes with this software, it is provided AS-IS.
////  *
////  ******************************************************************************
////  */
/////* USER CODE END Header */
////
/////* Includes ------------------------------------------------------------------*/
////#include "main.h"
////#include "stm32f429i_discovery.h"
////#include "stm32f429i_discovery_lcd.h"
////#include "stm32f429i_discovery_sdram.h"
////
/////* Private variables ---------------------------------------------------------*/
////
/////* Private function prototypes -----------------------------------------------*/
////void SystemClock_Config(void);
////static void MX_GPIO_Init(void);
////static void MX_FMC_Init(void);
////static void MX_LTDC_Init(void);
////
/////**
////  * @brief  The application entry point.
////  * @retval int
////  */
////
/////* Add these definitions to your main.c or keep them in separate files as generated by CubeMX */
////
/////**
////  * @brief FMC initialization function
////  * @retval None
////  */
////void MX_FMC_Init(void)
////{
////  /* Your FMC initialization code generated by CubeMX should be here */
////  /* Typically includes SDRAM controller configuration */
////}
////
/////**
////  * @brief LTDC initialization function
////  * @retval None
////  */
////void MX_LTDC_Init(void)
////{
////  /* Your LTDC initialization code generated by CubeMX should be here */
////  /* Typically includes display timing configuration */
////}
////
////int main(void)
////{
////  /* HAL initialization */
////  HAL_Init();
////
////  /* Configure the system clock */
////  SystemClock_Config();
////
////  /* Initialize all configured peripherals */
////  MX_GPIO_Init();
////  MX_FMC_Init();
////  MX_LTDC_Init();
////
////  /* Enable backlight */
////  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
////
////  /* Initialize SDRAM */
////  BSP_SDRAM_Init();
////
////  /* Test SDRAM - critical! */
////  uint32_t *testAddr = (uint32_t*)LCD_FRAME_BUFFER;
////  for(int i = 0; i < 1000; i++) {
////      testAddr[i] = 0x55555555;
////  }
////  for(int i = 0; i < 1000; i++) {
////      if(testAddr[i] != 0x55555555) {
////          Error_Handler(); // SDRAM test failed
////      }
////  }
////
////  /* Initialize LCD with proper delays */
////  BSP_LCD_Init();
////  HAL_Delay(100);  // Important delay for LCD power stabilization
////
////  /* Configure Layer 0 */
////  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
////  BSP_LCD_SetLayerVisible(0, ENABLE);
////  BSP_LCD_SetTransparency(0, 255);
////  BSP_LCD_SelectLayer(0);
////
////  /* Clear screen with black */
////  BSP_LCD_Clear(LCD_COLOR_BLACK);
////  HAL_Delay(500);
////
////  /* Display test pattern */
////  for(int y = 0; y < BSP_LCD_GetYSize(); y++) {
////      for(int x = 0; x < BSP_LCD_GetXSize(); x++) {
////          uint16_t color = ((x/16 + y/16) % 2) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
////          BSP_LCD_DrawPixel(x, y, color);
////      }
////  }
////  HAL_Delay(1000);
////
////  /* Clear and display text */
////  BSP_LCD_Clear(LCD_COLOR_BLUE);
////  BSP_LCD_SetFont(&Font24);
////  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
////  BSP_LCD_DisplayStringAt(0, LINE(5), (uint8_t *)"HELLO", CENTER_MODE);
////
////  /* Main loop */
////  while (1)
////  {
////      HAL_Delay(500);
////      BSP_LED_Toggle(LED4);  // Visual indicator that code is running
////  }
////}
////
/////**
////  * @brief System Clock Configuration
////  * @retval None
////  */
////void SystemClock_Config(void)
////{
////  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
////  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
////
////  /** Configure the main internal regulator output voltage */
////  __HAL_RCC_PWR_CLK_ENABLE();
////  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  // Changed to SCALE1 for better performance
////
////  /** Initializes the RCC Oscillators according to the specified parameters */
////  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
////  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
////  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
////  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
////  RCC_OscInitStruct.PLL.PLLM = 8;
////  RCC_OscInitStruct.PLL.PLLN = 336;
////  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
////  RCC_OscInitStruct.PLL.PLLQ = 7;
////  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
////  {
////    Error_Handler();
////  }
////
////  /** Initializes the CPU, AHB and APB buses clocks */
////  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
////                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
////  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
////  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
////  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
////  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
////
////  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
////  {
////    Error_Handler();
////  }
////}
////
/////**
////  * @brief GPIO Initialization Function
////  * @param None
////  * @retval None
////  */
////static void MX_GPIO_Init(void)
////{
////  GPIO_InitTypeDef GPIO_InitStruct = {0};
////
////  /* GPIO Ports Clock Enable */
////  __HAL_RCC_GPIOA_CLK_ENABLE();
////
////  /* Configure GPIO pin Output Level */
////  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
////
////  /* Configure GPIO pin : PA3 (LCD Backlight) */
////  GPIO_InitStruct.Pin = GPIO_PIN_3;
////  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
////  GPIO_InitStruct.Pull = GPIO_NOPULL;
////  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
////  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
////}
////
/////* FMC and LTDC initialization functions would be generated by CubeMX */
////
/////**
////  * @brief  This function is executed in case of error occurrence.
////  * @retval None
////  */
////void Error_Handler(void)
////{
////  __disable_irq();
////  while (1)
////  {
////    BSP_LED_Toggle(LED3);  // Visual error indicator
////    HAL_Delay(100);
////  }
////}
////
////#ifdef USE_FULL_ASSERT
/////**
////  * @brief  Reports the name of the source file and the source line number
////  *         where the assert_param error has occurred.
////  * @param  file: pointer to the source file name
////  * @param  line: assert_param error line source number
////  * @retval None
////  */
////void assert_failed(uint8_t *file, uint32_t line)
////{
////  /* User can add his own implementation to report the file name and line number */
////}
////#endif /* USE_FULL_ASSERT */
//
//
//
///* Includes ------------------------------------------------------------------*/
//#include "main.h"
//#include "stm32f429i_discovery.h"
//#include "stm32f429i_discovery_lcd.h"
//#include "stm32f429i_discovery_sdram.h"
//
//extern SDRAM_HandleTypeDef hsdram1;
//extern LTDC_HandleTypeDef hltdc;
//
//
//
///* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
//static void MX_GPIO_Init(void);
//static void MX_FMC_Init(void);
//static void MX_LTDC_Init(void);
//
//
//
//int main(void)
//{
//  /* HAL initialization */
//  HAL_Init();
//
//  /* Configure the system clock */
//  SystemClock_Config();
//
//  /* Initialize peripherals */
//  MX_GPIO_Init();
//  MX_FMC_Init();
//  MX_LTDC_Init();
//
//  /* Initialize LEDs */
//  BSP_LED_Init(LED3);
//  BSP_LED_Init(LED4);
//
//  /* Enable LCD backlight (PA3) */
//  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
//
//  /* Initialize SDRAM via BSP driver */
//  if (BSP_SDRAM_Init() != SDRAM_OK) {
//    Error_Handler();
//  }
//
//  /* SDRAM self-test */
//  uint32_t *testAddr = (uint32_t*)LCD_FRAME_BUFFER;
//  for(int i = 0; i < 1000; i++) testAddr[i] = 0x55555555;
//  for(int i = 0; i < 1000; i++) {
//    if(testAddr[i] != 0x55555555) {
//      Error_Handler(); // SDRAM error → LED3 blinks
//    }
//  }
//
//  /* Initialize LCD */
//  BSP_LCD_Init();
//  HAL_Delay(100);
//
//  /* Configure Layer 0 */
//  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
//  BSP_LCD_SetLayerVisible(0, ENABLE);
//  BSP_LCD_SetTransparency(0, 255);
//  BSP_LCD_SelectLayer(0);
//
//  /* Clear screen */
//  BSP_LCD_Clear(LCD_COLOR_BLACK);
//  HAL_Delay(500);
//
//  /* Checkerboard test */
//  for(int y = 0; y < BSP_LCD_GetYSize(); y++) {
//    for(int x = 0; x < BSP_LCD_GetXSize(); x++) {
//      uint16_t color = ((x/16 + y/16) % 2) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
//      BSP_LCD_DrawPixel(x, y, color);
//    }
//  }
//  HAL_Delay(1000);
//
//  /* Show text */
//  BSP_LCD_Clear(LCD_COLOR_BLUE);
//  BSP_LCD_SetFont(&Font24);
//  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
//  BSP_LCD_DisplayStringAt(0, LINE(5), (uint8_t *)"HELLO", CENTER_MODE);
//
//  /* Main loop: heartbeat LED4 */
//  while (1)
//  {
//    BSP_LED_Toggle(LED4);
//    HAL_Delay(500);
//  }
//}
//
///* -------------------------------------------------------------------------- */
///* System Clock Configuration: 168 MHz HSE + PLL                              */
///* -------------------------------------------------------------------------- */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//
//  __HAL_RCC_PWR_CLK_ENABLE();
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
//
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
//  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
//  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
//  RCC_OscInitStruct.PLL.PLLM = 8;
//  RCC_OscInitStruct.PLL.PLLN = 336;
//  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
//  RCC_OscInitStruct.PLL.PLLQ = 7;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
//
//  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
//                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
//  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
//  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
//  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
//
//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
//}
//
///* -------------------------------------------------------------------------- */
///* GPIO Init (PA3 = LCD Backlight)                                            */
///* -------------------------------------------------------------------------- */
//static void MX_GPIO_Init(void)
//{
//  GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//
//  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
//
//  GPIO_InitStruct.Pin = GPIO_PIN_3;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//}
//
///* -------------------------------------------------------------------------- */
///* FMC / SDRAM Init for IS42S16400J (8MB, 16-bit)                             */
///* -------------------------------------------------------------------------- */
//static void MX_FMC_Init(void)
//{
//  FMC_SDRAM_TimingTypeDef SdramTiming = {0};
//
//  hsdram1.Instance = FMC_SDRAM_DEVICE;
//  hsdram1.Init.SDBank             = FMC_SDRAM_BANK1;
//  hsdram1.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
//  hsdram1.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
//  hsdram1.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_16;
//  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
//  hsdram1.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
//  hsdram1.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
//  hsdram1.Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2;
//  hsdram1.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
//  hsdram1.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_1;
//
//  SdramTiming.LoadToActiveDelay    = 2;
//  SdramTiming.ExitSelfRefreshDelay = 7;
//  SdramTiming.SelfRefreshTime      = 4;
//  SdramTiming.RowCycleDelay        = 7;
//  SdramTiming.WriteRecoveryTime    = 2;
//  SdramTiming.RPDelay              = 2;
//  SdramTiming.RCDDelay             = 2;
//
//  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK) {
//    Error_Handler();
//  }
//}
//
///* -------------------------------------------------------------------------- */
///* LTDC Init for 240x320 TFT LCD (RGB565)                                     */
///* -------------------------------------------------------------------------- */
//static void MX_LTDC_Init(void)
//{
//  LTDC_LayerCfgTypeDef pLayerCfg = {0};
//
//  hltdc.Instance = LTDC;
//  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
//  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
//  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
//  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
//
//  hltdc.Init.HorizontalSync = 9;
//  hltdc.Init.VerticalSync = 1;
//  hltdc.Init.AccumulatedHBP = 29;
//  hltdc.Init.AccumulatedVBP = 3;
//  hltdc.Init.AccumulatedActiveW = 269;
//  hltdc.Init.AccumulatedActiveH = 323;
//  hltdc.Init.TotalWidth = 279;
//  hltdc.Init.TotalHeigh = 327;
//
//  hltdc.Init.Backcolor.Red = 0;
//  hltdc.Init.Backcolor.Green = 0;
//  hltdc.Init.Backcolor.Blue = 0;
//
//  if (HAL_LTDC_Init(&hltdc) != HAL_OK) {
//    Error_Handler();
//  }
//
//  pLayerCfg.WindowX0 = 0;
//  pLayerCfg.WindowX1 = 240;
//  pLayerCfg.WindowY0 = 0;
//  pLayerCfg.WindowY1 = 320;
//  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
//  pLayerCfg.FBStartAdress = LCD_FRAME_BUFFER;
//  pLayerCfg.Alpha = 255;
//  pLayerCfg.Alpha0 = 0;
//  pLayerCfg.Backcolor.Red = 0;
//  pLayerCfg.Backcolor.Green = 0;
//  pLayerCfg.Backcolor.Blue = 0;
//  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
//  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
//  pLayerCfg.ImageWidth = 240;
//  pLayerCfg.ImageHeight = 320;
//
//  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK) {
//    Error_Handler();
//  }
//}
//
///* -------------------------------------------------------------------------- */
///* Error Handler                                                              */
///* -------------------------------------------------------------------------- */
//void Error_Handler(void)
//{
//  __disable_irq();
//  while (1) {
//    BSP_LED_Toggle(LED3);
//    HAL_Delay(100);
//  }
//}



/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* Infinite loop */
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Toggle LED4 (PC13)
    HAL_Delay(500); // 500 ms delay
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Initialize pin state (LED OFF initially) */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
