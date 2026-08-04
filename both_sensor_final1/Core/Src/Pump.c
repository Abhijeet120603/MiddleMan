/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : pump.c
  * @brief          : Pump control implementation using TIM3 PWM
  *                   This file contains the pump motor control functions.
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
#include "pump.h"

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#define PUMP_TIMER_AUTO_RELOAD  1000  /* 1000 ticks for 1% resolution */
#define PWM_MAX_PERCENTAGE      100
#define PWM_MIN_PERCENTAGE      0

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef *pump_timer = NULL;
static uint8_t pump_running = 0;
static uint8_t current_speed = 0;
static Pump_Direction_t current_direction = PUMP_FORWARD;

/* Private function prototypes -----------------------------------------------*/
static void PUMP_SetPWM(uint8_t percentage);
static void PUMP_EnableOutputs(void);
static void PUMP_DisableOutputs(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize the pump control
  * @param  htim: Timer handle for PWM generation
  * @retval None
  */
void PUMP_Init(TIM_HandleTypeDef *htim)
{
    pump_timer = htim;
    pump_running = 0;
    current_speed = 0;
    current_direction = PUMP_FORWARD;

    /* Stop PWM initially */
    PUMP_Stop();
}

/**
  * @brief  Move the pump in specified direction with given speed and duration
  * @param  direction: PUMP_FORWARD or PUMP_REVERSE
  * @param  duration_sec: Duration in seconds (0 = continuous)
  * @param  pwm_percentage: PWM duty cycle percentage (0-100)
  * @retval None
  */
void PUMP_Move(Pump_Direction_t direction, uint16_t duration_sec, uint8_t pwm_percentage)
{
    if (pump_timer == NULL) {
        return; /* Pump not initialized */
    }

    if (pwm_percentage > PWM_MAX_PERCENTAGE) {
        pwm_percentage = PWM_MAX_PERCENTAGE;
    }

    /* Stop any ongoing pump operation */
    PUMP_Stop();

    /* Set direction */
    current_direction = direction;
    current_speed = pwm_percentage;

    /* Configure direction GPIO */
    if (direction == PUMP_FORWARD) {
        /* Pump Forward - Channel 1 */
        HAL_GPIO_WritePin(Pump_Forward_TIM3_CH1_GPIO_Port, Pump_Forward_TIM3_CH1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(Pump_Reverse_TIM3_CH1_GPIO_Port, Pump_Reverse_TIM3_CH1_Pin, GPIO_PIN_RESET);
    } else {
        /* Pump Reverse - Channel 2 */
        HAL_GPIO_WritePin(Pump_Forward_TIM3_CH1_GPIO_Port, Pump_Forward_TIM3_CH1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(Pump_Reverse_TIM3_CH1_GPIO_Port, Pump_Reverse_TIM3_CH1_Pin, GPIO_PIN_SET);
    }

    /* Set PWM duty cycle */
    PUMP_SetPWM(pwm_percentage);

    /* Enable PWM outputs */
    PUMP_EnableOutputs();

    pump_running = 1;

    /* Handle duration if specified */
    if (duration_sec > 0) {
        /* Use OS delay instead of HAL_Delay */
        osDelay(duration_sec * 1000); /* Convert seconds to milliseconds */

        /* Stop after duration */
        PUMP_Stop();
    }
}

/**
  * @brief  Stop the pump immediately
  * @retval None
  */
void PUMP_Stop(void)
{
    if (pump_timer == NULL) {
        return;
    }

    /* Disable PWM outputs */
    PUMP_DisableOutputs();

    /* Set PWM to 0% */
    PUMP_SetPWM(0);

    /* Turn off both direction pins */
    HAL_GPIO_WritePin(Pump_Forward_TIM3_CH1_GPIO_Port, Pump_Forward_TIM3_CH1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Pump_Reverse_TIM3_CH1_GPIO_Port, Pump_Reverse_TIM3_CH1_Pin, GPIO_PIN_RESET);

    pump_running = 0;
    current_speed = 0;
}

/**
  * @brief  Set PWM duty cycle
  * @param  percentage: PWM duty cycle percentage (0-100)
  * @retval None
  */
void PUMP_SetSpeed(uint8_t percentage)
{
    if (pump_timer == NULL) {
        return;
    }

    if (percentage > PWM_MAX_PERCENTAGE) {
        percentage = PWM_MAX_PERCENTAGE;
    }

    current_speed = percentage;

    if (pump_running) {
        PUMP_SetPWM(percentage);
    }
}

/**
  * @brief  Check if pump is currently running
  * @retval 1 if running, 0 if stopped
  */
uint8_t PUMP_IsRunning(void)
{
    return pump_running;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Set PWM duty cycle on TIM3
  * @param  percentage: PWM duty cycle percentage (0-100)
  * @retval None
  */
static void PUMP_SetPWM(uint8_t percentage)
{
    if (pump_timer == NULL) {
        return;
    }

    uint32_t pulse_value = 0;

    if (percentage > 0) {
        /* Calculate pulse value based on percentage */
        pulse_value = (PUMP_TIMER_AUTO_RELOAD * percentage) / 100;
    }

    /* Set the compare value for the active channel */
    if (current_direction == PUMP_FORWARD) {
        /* Set PWM on Pump_Forward channel */
        __HAL_TIM_SET_COMPARE(pump_timer, TIM_CHANNEL_1, pulse_value);
    } else {
        /* Set PWM on Pump_Reverse channel */
        __HAL_TIM_SET_COMPARE(pump_timer, TIM_CHANNEL_2, pulse_value);
    }
}

/**
  * @brief  Enable PWM outputs
  * @retval None
  */
static void PUMP_EnableOutputs(void)
{
    if (pump_timer == NULL) {
        return;
    }

    /* Enable PWM output */
    HAL_TIM_PWM_Start(pump_timer, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(pump_timer, TIM_CHANNEL_2);
}

/**
  * @brief  Disable PWM outputs
  * @retval None
  */
static void PUMP_DisableOutputs(void)
{
    if (pump_timer == NULL) {
        return;
    }

    /* Disable PWM output */
    HAL_TIM_PWM_Stop(pump_timer, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(pump_timer, TIM_CHANNEL_2);
}

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
