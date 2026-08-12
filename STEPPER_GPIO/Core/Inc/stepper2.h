/*
 * stepper2.h
 *
 *  Created on: Aug 11, 2026
 *      Author: HP
 */

#ifndef INC_STEPPER2_H_
#define INC_STEPPER2_H_

#include "main.h"

/* Stepper motor configuration */
#define STEPPER_STEPS_PER_REV     200      /* 200 steps per revolution for typical stepper */
#define STEPPER_DEFAULT_SPEED     1000     /* Steps per second */
#define STEPPER_HOME_OFFSET       100      /* Steps to move forward after homing */

/* Stepper direction definitions */
#define STEPPER_DIR_CW            GPIO_PIN_RESET
#define STEPPER_DIR_CCW           GPIO_PIN_SET

/* Stepper states */
typedef enum {
    STEPPER_IDLE,
    STEPPER_MOVING,
    STEPPER_HOMING,
    STEPPER_AT_HOME
} Stepper_State_t;

/* Function prototypes */
void STEPPER_Init(void);
void STEPPER_Home(void);
void STEPPER_MoveSteps(uint32_t steps, uint8_t direction);
void STEPPER_MoveContinuous(uint8_t direction);
void STEPPER_Stop(void);
void STEPPER_Enable(void);
void STEPPER_Disable(void);
uint8_t STEPPER_IsHomed(void);
Stepper_State_t STEPPER_GetState(void);
void STEPPER_SetSpeed(uint32_t steps_per_second);

/* User callbacks */
void STEPPER_HomingCompleteCallback(void);


#endif /* INC_STEPPER2_H_ */
