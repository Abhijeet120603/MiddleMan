/*
 * Stepper.h
 *
 *  Created on: Aug 1, 2026
 *      Author: HP
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

//extern volatile uint8_t GrooveSensorState;
///* Function Prototypes */
//bool Stepper_IsGrooveDetected(void);
//
//void Stepper_Enable(void);
//void Stepper_Disable(void);
//
//void Stepper_MovePositive(uint32_t steps);
//void Stepper_MoveNegative(uint32_t steps);
//
//void Stepper_GrooveSequence(void);
//
//void Stepper_TimerPulseFinishedCallback(TIM_HandleTypeDef *htim);

typedef enum {
    POSITIVE = GPIO_PIN_SET,
    NEGATIVE = GPIO_PIN_RESET
} StepperDirection_t;

typedef enum {
    MICROSTEP_FULL,
    MICROSTEP_HALF,
    MICROSTEP_QUARTER,
    MICROSTEP_EIGHTH,
    MICROSTEP_SIXTEENTH
} MicrostepMode_t;

/* Extern Variables */
extern volatile uint8_t GrooveSensorState;
extern volatile HAL_StatusTypeDef StepperPWMStatus;

/* Function Prototypes */

/* Basic Control Functions */
bool Stepper_IsGrooveDetected(void);

void Stepper_Enable(void);
void Stepper_Disable(void);

void Stepper_SetMicrostepping(MicrostepMode_t mode);

/* Movement Functions - Direct Control */
void Stepper_MoveForward(uint32_t duration_ms, uint32_t step_frequency);
void Stepper_MoveReverse(uint32_t duration_ms, uint32_t step_frequency);
void Stepper_Move(StepperDirection_t dir, uint32_t duration_ms, uint32_t step_frequency);

/* Movement Functions - Step Based (with interrupt) */
void Stepper_MovePositive(uint32_t steps, uint32_t step_frequency);
void Stepper_MoveNegative(uint32_t steps, uint32_t step_frequency);

/* Alignment and Homing Functions */
void Stepper_Home(void);
void Stepper_UV_Sensor_Align(void);
void Stepper_White_LED_Align(void);

/* Sequence Functions */
void Stepper_GrooveSequence(void);

/* Callback Functions */
void Stepper_TimerPulseFinishedCallback(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* INC_STEPPER_H_ */
