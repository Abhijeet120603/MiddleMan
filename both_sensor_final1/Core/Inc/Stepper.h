/*
 * stepper.h
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

typedef enum {
    POSITIVE = GPIO_PIN_SET,
    NEGATIVE = GPIO_PIN_RESET
} StepperDirection_t;

/* Extern Variables */
extern volatile uint8_t GrooveSensorState;
extern volatile uint32_t StepCount;
extern volatile uint8_t MotorRunning;

/* Function Prototypes */

/* Basic Control Functions */
bool Stepper_IsGrooveDetected(void);
void Stepper_Enable(void);
void Stepper_Disable(void);

/* GPIO Stepping Functions */
void Stepper_Step(void);
void Stepper_DelayMicroseconds(uint32_t us);

/* Movement Functions - GPIO Based */
void Stepper_MoveForward(uint32_t steps, uint32_t delay_ms);
void Stepper_MoveReverse(uint32_t steps, uint32_t delay_ms);
void Stepper_Move(StepperDirection_t dir, uint32_t steps, uint32_t delay_ms);

/* Alignment and Homing Functions */
void Stepper_Home(void);
void Stepper_UV_Sensor_Align(void);
void Stepper_White_LED_Align(void);

/* Test Functions */
void Stepper_Test(void);

/* Sequence Functions */
void Stepper_GrooveSequence(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_STEPPER_H_ */
