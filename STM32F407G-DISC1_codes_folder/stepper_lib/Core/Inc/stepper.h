#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "stm32f4xx_hal.h"

#define STEPPER_DIR_PORT GPIOA
#define STEPPER_DIR_PIN  GPIO_PIN_2

#define STEPPER_EN_PORT  GPIOA
#define STEPPER_EN_PIN   GPIO_PIN_4

#define STEPPER_TIM_CHANNEL TIM_CHANNEL_2

void Stepper_Init(TIM_HandleTypeDef *htim);
void Stepper_Start(uint8_t dir, uint32_t duration, uint32_t speed);
void Stepper_Update(void);

#endif


















//#ifndef INC_STEPPER_H_
//#define INC_STEPPER_H_
//
//#include "stm32f4xx_hal.h"
//
///* ===== USER CONFIGURATION ===== */
//#define STEPPER_DIR_PORT     GPIOA
//#define STEPPER_DIR_PIN      GPIO_PIN_2
//
//#define STEPPER_EN_PORT      GPIOA
//#define STEPPER_EN_PIN       GPIO_PIN_4
//
//#define STEPPER_TIM_CHANNEL  TIM_CHANNEL_2
///* ============================== */
//
//void Stepper_Init(TIM_HandleTypeDef *htim);
//void Stepper_Start(uint8_t direction, uint32_t duration_sec, uint32_t speed_hz);
//void Stepper_Stop(void);
//void Stepper_Update(void);
//
//#endif
//
//
//
//
//
//
//
