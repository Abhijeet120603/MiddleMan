/*
 * stepper.c
 *
 *  Created on: Aug 11, 2026
 *      Author: STM32F4
 */

#include "stepper.h"
#include "tim.h"          /* For HAL timer functions */

/* Private variables */
static Stepper_State_t stepper_state = STEPPER_IDLE;
static uint8_t is_homed = 0;
static uint32_t target_steps = 0;
static uint32_t current_step_count = 0;
static uint32_t step_delay_us = 1000;  /* Default: 1000us = 1000 steps/sec */
static uint8_t homing_done = 0;

/* Private function prototypes */
static void STEPPER_GeneratePulse(void);
static uint8_t STEPPER_IsGrooveSensorActive(void);

/**
  * @brief  Initialize stepper motor hardware
  * @param  None
  * @retval None
  */
void STEPPER_Init(void)
{
    /* Enable stepper driver */
    STEPPER_Enable();

    /* Set initial direction */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, STEPPER_DIR_CW);

    /* Initialize state */
    stepper_state = STEPPER_IDLE;
    is_homed = 0;
    current_step_count = 0;

    /* Configure TIM for step generation */
    /* Assuming TIM2 is used for step generation */
    __HAL_TIM_SET_AUTORELOAD(&htim2, step_delay_us - 1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (step_delay_us / 2));
}

/**
  * @brief  Home the stepper motor
  * @param  None
  * @retval None
  */
void STEPPER_Home(void)
{
    if (stepper_state != STEPPER_IDLE) {
        return;  /* Motor is busy */
    }

    stepper_state = STEPPER_HOMING;
    is_homed = 0;

    /* Start moving towards groove sensor (CCW direction) */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, STEPPER_DIR_CCW);

    /* Check if already at home sensor */
    if (STEPPER_IsGrooveSensorActive()) {
        /* If sensor active, move forward 100 steps (as required) */
        STEPPER_MoveSteps(STEPPER_HOME_OFFSET, STEPPER_DIR_CW);
        stepper_state = STEPPER_AT_HOME;
        is_homed = 1;

        /* Call completion callback */
        STEPPER_HomingCompleteCallback();
        return;
    }

    /* Move continuously towards sensor */
    STEPPER_MoveContinuous(STEPPER_DIR_CCW);
}

/**
  * @brief  Move stepper motor by specified number of steps
  * @param  steps: Number of steps to move
  * @param  direction: STEPPER_DIR_CW or STEPPER_DIR_CCW
  * @retval None
  */
void STEPPER_MoveSteps(uint32_t steps, uint8_t direction)
{
    if (stepper_state != STEPPER_IDLE && stepper_state != STEPPER_AT_HOME) {
        return;  /* Motor is busy */
    }

    /* Set direction */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);

    target_steps = steps;
    current_step_count = 0;
    stepper_state = STEPPER_MOVING;

    /* Start timer to generate step pulses */
    /* Enable the timer channel */
    HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
}

/**
  * @brief  Move stepper motor continuously in a direction
  * @param  direction: STEPPER_DIR_CW or STEPPER_DIR_CCW
  * @retval None
  */
void STEPPER_MoveContinuous(uint8_t direction)
{
    if (stepper_state == STEPPER_MOVING) {
        return;  /* Already moving */
    }

    /* Set direction */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);

    target_steps = 0xFFFFFFFF;  /* Infinite steps */
    stepper_state = STEPPER_MOVING;

    /* Start timer for continuous pulses */
    HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
}

/**
  * @brief  Stop stepper motor movement
  * @param  None
  * @retval None
  */
void STEPPER_Stop(void)
{
    /* Stop timer */
    HAL_TIM_OC_Stop_IT(&htim2, TIM_CHANNEL_1);

    stepper_state = STEPPER_IDLE;
}

/**
  * @brief  Enable stepper driver
  * @param  None
  * @retval None
  */
void STEPPER_Enable(void)
{
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);  /* Enable active low */
}

/**
  * @brief  Disable stepper driver
  * @param  None
  * @retval None
  */
void STEPPER_Disable(void)
{
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);    /* Disable active high */
}

/**
  * @brief  Check if motor is homed
  * @param  None
  * @retval 1 if homed, 0 if not
  */
uint8_t STEPPER_IsHomed(void)
{
    return is_homed;
}

/**
  * @brief  Get current stepper state
  * @param  None
  * @retval Current state
  */
Stepper_State_t STEPPER_GetState(void)
{
    return stepper_state;
}

/**
  * @brief  Set stepper speed
  * @param  steps_per_second: Steps per second
  * @retval None
  */
void STEPPER_SetSpeed(uint32_t steps_per_second)
{
    if (steps_per_second == 0) return;

    step_delay_us = 1000000 / steps_per_second;  /* Convert to microseconds */

    /* Update timer */
    __HAL_TIM_SET_AUTORELOAD(&htim2, step_delay_us - 1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (step_delay_us / 2));
}

/**
  * @brief  Generate a single step pulse (called from timer interrupt)
  * @param  None
  * @retval None
  */
static void STEPPER_GeneratePulse(void)
{
    if (stepper_state != STEPPER_MOVING && stepper_state != STEPPER_HOMING) {
        return;
    }

    /* Generate pulse */
    HAL_GPIO_TogglePin(STEP_GPIO_Port, STEP_Pin);
    HAL_GPIO_TogglePin(STEP_GPIO_Port, STEP_Pin);  /* Create pulse */

    current_step_count++;

    /* Check if homing and reached sensor */
    if (stepper_state == STEPPER_HOMING) {
        if (STEPPER_IsGrooveSensorActive()) {
            /* Sensor reached - stop and move forward 100 steps */
            STEPPER_Stop();

            /* Now move forward 100 steps (as required) */
            STEPPER_MoveSteps(STEPPER_HOME_OFFSET, STEPPER_DIR_CW);

            stepper_state = STEPPER_AT_HOME;
            is_homed = 1;

            /* Call completion callback */
            STEPPER_HomingCompleteCallback();
            return;
        }
    }

    /* Check if target steps reached */
    if (target_steps != 0xFFFFFFFF && current_step_count >= target_steps) {
        STEPPER_Stop();
    }
}

/**
  * @brief  Check groove sensor status
  * @param  None
  * @retval 1 if sensor active, 0 if inactive
  */
static uint8_t STEPPER_IsGrooveSensorActive(void)
{
    /* GROOVE_IN_Pin active low or high? Check your hardware */
    /* Assuming active low - change GPIO_PIN_RESET to GPIO_PIN_SET if active high */
    return (HAL_GPIO_ReadPin(GROOVE_IN_GPIO_Port, GROOVE_IN_Pin) == GPIO_PIN_RESET);
}

/**
  * @brief  Weak callback function for homing completion
  * @param  None
  * @retval None
  */
__weak void STEPPER_HomingCompleteCallback(void)
{
    /* User can override this function in main.c */
}

/**
  * @brief  Timer interrupt callback for step generation
  * @param  htim: Timer handle
  * @retval None
  */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        STEPPER_GeneratePulse();
    }
}
