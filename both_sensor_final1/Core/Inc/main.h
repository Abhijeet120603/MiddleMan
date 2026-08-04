/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define STEP_TIM2_CH1_Pin GPIO_PIN_0
#define STEP_TIM2_CH1_GPIO_Port GPIOA
#define EN_Pin GPIO_PIN_1
#define EN_GPIO_Port GPIOA
#define DIR_Pin GPIO_PIN_2
#define DIR_GPIO_Port GPIOA
#define MS1_Pin GPIO_PIN_3
#define MS1_GPIO_Port GPIOA
#define MS2_Pin GPIO_PIN_4
#define MS2_GPIO_Port GPIOA
#define Groove_Sensor_In_Pin GPIO_PIN_5
#define Groove_Sensor_In_GPIO_Port GPIOA
#define Pump_Forward_TIM3_CH1_Pin GPIO_PIN_6
#define Pump_Forward_TIM3_CH1_GPIO_Port GPIOA
#define Pump_Reverse_TIM3_CH1_Pin GPIO_PIN_7
#define Pump_Reverse_TIM3_CH1_GPIO_Port GPIOA
#define White_LED_Pin GPIO_PIN_14
#define White_LED_GPIO_Port GPIOB
#define UV_LED_Pin GPIO_PIN_15
#define UV_LED_GPIO_Port GPIOB
#define LTR390_SCL_Pin GPIO_PIN_8
#define LTR390_SCL_GPIO_Port GPIOA
#define LTR390_SDA_Pin GPIO_PIN_4
#define LTR390_SDA_GPIO_Port GPIOB
#define AS7341_SCL_Pin GPIO_PIN_6
#define AS7341_SCL_GPIO_Port GPIOB
#define AS7341_SDA_Pin GPIO_PIN_7
#define AS7341_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
