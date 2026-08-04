/*
 * Pump.h
 *
 *  Created on: Aug 4, 2026
 *      Author: HP
 */

#ifndef INC_PUMP_H_
#define INC_PUMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    PUMP_FORWARD = 0,
    PUMP_REVERSE = 1
} Pump_Direction_t;

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  Initialize the pump timer and GPIO
  * @param  htim: Timer handle for PWM generation
  * @retval None
  */
void PUMP_Init(TIM_HandleTypeDef *htim);

/**
  * @brief  Move the pump in specified direction with given speed and duration
  * @param  direction: PUMP_FORWARD or PUMP_REVERSE
  * @param  duration_sec: Duration in seconds (0 = continuous)
  * @param  pwm_percentage: PWM duty cycle percentage (0-100)
  * @retval None
  */
void PUMP_Move(Pump_Direction_t direction, uint16_t duration_sec, uint8_t pwm_percentage);

/**
  * @brief  Stop the pump immediately
  * @retval None
  */
void PUMP_Stop(void);

/**
  * @brief  Set PWM duty cycle
  * @param  percentage: PWM duty cycle percentage (0-100)
  * @retval None
  */
void PUMP_SetSpeed(uint8_t percentage);

/**
  * @brief  Check if pump is currently running
  * @retval 1 if running, 0 if stopped
  */
uint8_t PUMP_IsRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_PUMP_H_ */
