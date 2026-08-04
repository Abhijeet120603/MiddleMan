/*
 * stepper.h
 *
 *  Created on: Dec 18, 2025
 *      Author: abhij
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "stm32f4xx_hal.h"

/* ================= USER CONFIG ================= */
/* Timer used for STEP signal */
extern TIM_HandleTypeDef htim2;
extern volatile uint8_t stepper_busy;

/* GPIO definitions (from CubeMX) */
#define STEPPER_DIR_PORT   GPIOA
#define STEPPER_DIR_PIN    GPIO_PIN_2

#define STEPPER_EN_PORT    GPIOA
#define STEPPER_EN_PIN     GPIO_PIN_4
/* =============================================== */

/* Direction definitions */
#define STEPPER_FORWARD    1
#define STEPPER_REVERSE    0

/* API functions */
void stepper_set_speed_hz(uint32_t step_hz);
void stepper_enable(void);
void stepper_disable(void);
void stepper_set_direction(uint8_t direction);
void stepper_move(uint8_t direction,
                  uint32_t duration_sec,
                  uint32_t steps_per_second);



#endif /* INC_STEPPER_H_ */
