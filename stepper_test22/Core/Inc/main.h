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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define En_Pin GPIO_PIN_1
#define En_GPIO_Port GPIOA
#define Dir_Pin GPIO_PIN_2
#define Dir_GPIO_Port GPIOA
#define MS1_Pin GPIO_PIN_3
#define MS1_GPIO_Port GPIOA
#define MS2_Pin GPIO_PIN_4
#define MS2_GPIO_Port GPIOA
#define ENABLE_Pin GPIO_PIN_1
#define ENABLE_GPIO_Port GPIOB
#define STEP_Pin GPIO_PIN_2
#define STEP_GPIO_Port GPIOB
#define DIR_Pin GPIO_PIN_10
#define DIR_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

// Define enums BEFORE function prototypes
typedef enum {
    NEGATIVE = 0,
    POSITIVE = 1
} StepperDirection_t;

typedef enum {
    MICROSTEP_FULL = 0,
    MICROSTEP_HALF = 1,
    MICROSTEP_QUARTER = 2,
    MICROSTEP_EIGHTH = 3,
    MICROSTEP_SIXTEENTH = 4
} MicrostepMode_t;

// Function prototypes
void Stepper_Enable(void);
void Stepper_Disable(void);
void Move_Stepper(StepperDirection_t dir, uint32_t duration_ms, uint32_t step_frequency);
//void Set_Microstepping(MicrostepMode_t mode);

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
