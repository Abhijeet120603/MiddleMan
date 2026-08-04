/*
 * stepper.c
 *
 *  Created on: Dec 18, 2025
 *      Author: abhij
 */


#include "stepper.h"

/**
 * @brief Set step frequency using TIM2 PWM
 * Timer clock = 1 MHz (Prescaler = 83)
 */
void stepper_set_speed_hz(uint32_t step_hz)
{
    if (step_hz == 0) return;

    uint32_t arr = (1000000 / step_hz) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, arr / 2); // 50% duty
}

/**
 * @brief Enable stepper driver (Active LOW)
 */
void stepper_enable(void)
{
    HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Disable stepper driver
 */
void stepper_disable(void)
{
    HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_SET);
}

/**
 * @brief Set motor direction
 */
void stepper_set_direction(uint8_t direction)
{
    if (direction == STEPPER_FORWARD)
    {
        HAL_GPIO_WritePin(STEPPER_DIR_PORT, STEPPER_DIR_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(STEPPER_DIR_PORT, STEPPER_DIR_PIN, GPIO_PIN_RESET);
    }
}

/**
 * @brief Move stepper for given duration and speed
 */
void stepper_move(uint8_t direction,
                  uint32_t duration_sec,
                  uint32_t steps_per_second)
{
    stepper_set_direction(direction);
    stepper_enable();
    stepper_set_speed_hz(steps_per_second);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_Delay(duration_sec * 1000);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);

    stepper_disable();
}
