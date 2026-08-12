/*
 * stepper.c
 *
 *  Created on: Aug 1, 2026
 *      Author: HP
 */

#include "stepper.h"
#include "cmsis_os.h"

/* Private Variables */
volatile uint8_t GrooveSensorState = 0;
volatile uint32_t StepCount = 0;
volatile uint8_t MotorRunning = 0;

/*===========================================================================
 * Basic Control Functions
 *===========================================================================*/

/**
  * @brief  Check if groove sensor is detected
  * @retval bool - true if groove detected, false otherwise
  */
bool Stepper_IsGrooveDetected(void)
{
    GrooveSensorState =
        (HAL_GPIO_ReadPin(Groove_Sensor_In_GPIO_Port,
                          Groove_Sensor_In_Pin) == GPIO_PIN_SET);

    return GrooveSensorState;
}

/**
  * @brief  Enable the stepper motor driver (Active LOW)
  * @retval None
  */
void Stepper_Enable(void)
{
    // EN is Active LOW on A4988/DRV8825/TMC2208
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);

    // MS1 and MS2 always HIGH for maximum microstepping
    HAL_GPIO_WritePin(MS1_GPIO_Port, MS1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MS2_GPIO_Port, MS2_Pin, GPIO_PIN_SET);

    // Small delay for driver to enable
    osDelay(1);
}

/**
  * @brief  Disable the stepper motor driver (Inactive HIGH)
  * @retval None
  */
void Stepper_Disable(void)
{
    // EN is Inactive HIGH
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);
}

/*===========================================================================
 * GPIO Stepping Functions
 *===========================================================================*/

/**
  * @brief  Microsecond delay using systick
  * @param  us: Microseconds to delay
  * @retval None
  */
void Stepper_DelayMicroseconds(uint32_t us)
{
    // Use HAL delay for microseconds (approximate)
    // For accurate microsecond delays, use a timer
    if (us < 1000) {
        // For delays less than 1ms, use a busy loop
        // Adjust this value based on your CPU speed
        // For 16MHz, each loop iteration takes about 4-5 cycles
        volatile uint32_t count = us * 4;
        while(count--);
    } else {
        // For longer delays, use HAL delay
        HAL_Delay(us / 1000);
    }
}

/**
  * @brief  Generate a single step pulse using GPIO
  * @retval None
  */
void Stepper_Step(void)
{
    // Step pulse: HIGH then LOW
    HAL_GPIO_WritePin(STEP_GPIO_Port, STEP_Pin, GPIO_PIN_SET);

    // Minimum pulse width for most drivers is 1-2us
    // Use a small busy loop for precise timing
    volatile uint32_t count = 10;
    while(count--);

    HAL_GPIO_WritePin(STEP_GPIO_Port, STEP_Pin, GPIO_PIN_RESET);

    // Small delay after pulse
    volatile uint32_t count2 = 5;
    while(count2--);
}

/*===========================================================================
 * Movement Functions - GPIO Based
 *===========================================================================*/

/**
  * @brief  Move stepper motor forward for specified number of steps
  * @param  steps: Number of steps to move
  * @param  delay_ms: Delay between steps in milliseconds
  * @retval None
  */
void Stepper_MoveForward(uint32_t steps, uint32_t delay_ms)
{
    Stepper_Move(POSITIVE, steps, delay_ms);
}

/**
  * @brief  Move stepper motor reverse for specified number of steps
  * @param  steps: Number of steps to move
  * @param  delay_ms: Delay between steps in milliseconds
  * @retval None
  */
void Stepper_MoveReverse(uint32_t steps, uint32_t delay_ms)
{
    Stepper_Move(NEGATIVE, steps, delay_ms);
}

/**
  * @brief  Move stepper motor in specified direction for number of steps
  * @param  dir: Direction to move (POSITIVE or NEGATIVE)
  * @param  steps: Number of steps to move
  * @param  delay_ms: Delay between steps in milliseconds
  * @retval None
  */
void Stepper_Move(StepperDirection_t dir, uint32_t steps, uint32_t delay_ms)
{
    // Ensure motor is enabled
    Stepper_Enable();

    // Set direction
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, (GPIO_PinState)dir);

    // Small delay after direction change
    osDelay(1);

    // Perform steps
    for(uint32_t i = 0; i < steps; i++)
    {
        Stepper_Step();

        // Wait between steps (in milliseconds)
        if(delay_ms > 0)
        {
            osDelay(delay_ms);
        }
    }

    // Disable motor after movement
    Stepper_Disable();
}

/*===========================================================================
 * Alignment and Homing Functions
 *===========================================================================*/

/**
  * @brief  Home4 function - moves toward groove, then back off 50 steps
  * @retval None
  */
//void Stepper_Home(void)
//{
//    uint32_t delay_ms = 40;     // 5ms between steps (200Hz) - SLOW and VISIBLE
//    uint32_t max_steps = 10000;
//    uint32_t steps_moved = 0;
//    uint8_t groove_found = 0;
//
//    // Enable driver
//    Stepper_Enable();
//    osDelay(10);
//
//    // Send start message via UART (optional)
//    // HAL_UART_Transmit(&huart1, (uint8_t *)"HOMING START\r\n", 14, HAL_MAX_DELAY);
//
//    // Move POSITIVE until groove sensor is detected
//    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
//    osDelay(1);
//
//    while(steps_moved < max_steps)
//    {
//        // Generate step pulse
//        Stepper_Step();
//        steps_moved++;
//
//        // Check if groove is detected
//        if(Stepper_IsGrooveDetected())
//        {
//            groove_found = 1;
//            break;
//        }
//
//        // Delay between steps
//        osDelay(delay_ms);
//    }
//
//    if(groove_found)
//    {
//        // Groove found - stop and back off
////        Stepper_Disable();
//        osDelay(1000);
////
////        // Move reverse for 50 steps (back off from groove)
////        Stepper_Enable();
//        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
//        osDelay(1);
//
//        for(uint32_t i = 0; i < 20; i++)
//        {
//            Stepper_Step();
//            osDelay(delay_ms);
//        }
//
//        // Send success message
//        // HAL_UART_Transmit(&huart1, (uint8_t *)"HOMING SUCCESS\r\n", 17, HAL_MAX_DELAY);
//    }
//    else
//    {
//        // Max steps reached without detecting groove
//        // Send failure message
//        // HAL_UART_Transmit(&huart1, (uint8_t *)"HOMING FAILED\r\n", 16, HAL_MAX_DELAY);
//    }
//
//    // Disable motor
//    Stepper_Disable();
//}

void Stepper_Home(void)
{
    uint32_t delay_fast = 20;       // First approach speed
    uint32_t delay_slow = 40;      // Second approach - much slower

    uint32_t max_steps = 10000;

    uint32_t steps_moved = 0;
    uint8_t groove_found = 0;

    /* =========================================================
     * FIRST APPROACH
     * Move TOWARDS groove
     * ========================================================= */

    Stepper_Enable();
    osDelay(10);

    /* Direction towards groove */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
    osDelay(10);

    steps_moved = 0;
    groove_found = 0;

    while (steps_moved < max_steps)
    {
        Stepper_Step();
        steps_moved++;

        /* Check groove */
        if (Stepper_IsGrooveDetected())
        {
            groove_found = 1;
            break;
        }

        osDelay(delay_fast);
    }


    /* =========================================================
     * GROOVE FOUND - FIRST BACK OFF
     * Move OPPOSITE direction 20 steps
     * ========================================================= */

    if (groove_found)
    {
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
        osDelay(10);

        for (uint32_t i = 0; i < 115; i++)
        {
            Stepper_Step();
            osDelay(delay_fast);
        }


        /* =====================================================
         * SECOND APPROACH
         * Move TOWARDS groove again, MUCH SLOWER
         * ===================================================== */

        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
        osDelay(10);

        steps_moved = 0;
        groove_found = 0;

        while (steps_moved < max_steps)
        {
            Stepper_Step();
            steps_moved++;

            /* Check groove again */
            if (Stepper_IsGrooveDetected())
            {
                groove_found = 1;
                break;
            }

            /* Much slower approach */
            osDelay(delay_slow);
        }


        /* =====================================================
         * SECOND BACK OFF
         * Move OPPOSITE direction 20 steps
         * ===================================================== */

        if (groove_found)
        {
            HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
            osDelay(10);

            for (uint32_t i = 0; i < 115; i++)
            {
                Stepper_Step();
                osDelay(delay_slow);
            }
        }
    }

    /* =========================================================
     * FINISH
     * ========================================================= */

    Stepper_Disable();
}

/**
  * @brief  Test function to move stepper with visible steps
  * @retval None
  */
void Stepper_Test(void)
{
    // Enable driver
    Stepper_Enable();
    osDelay(10);

    // Move forward 100 steps slowly (visible movement)
    Stepper_MoveForward(100, 10);

    osDelay(500);

    // Move reverse 100 steps slowly
    Stepper_MoveReverse(100, 10);

    Stepper_Disable();
}

/**
  * @brief  Align for UV sensor measurement
  * @retval None
  */
void Stepper_UV_Sensor_Align(void)
{
    uint32_t delay_fast = 20;       // First approach speed
    uint32_t delay_slow = 40;      // Second approach - much slower

    uint32_t max_steps = 10000;

    uint32_t steps_moved = 0;
    uint8_t groove_found = 0;

    /* =========================================================
     * FIRST APPROACH
     * Move TOWARDS groove
     * ========================================================= */

    Stepper_Enable();
    osDelay(10);

    /* Direction towards groove */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
    osDelay(10);

    steps_moved = 0;
    groove_found = 0;

    while (steps_moved < max_steps)
    {
        Stepper_Step();
        steps_moved++;

        /* Check groove */
        if (Stepper_IsGrooveDetected())
        {
            groove_found = 1;
            break;
        }

        osDelay(delay_fast);
    }


    /* =========================================================
     * GROOVE FOUND - FIRST BACK OFF
     * Move OPPOSITE direction 20 steps
     * ========================================================= */

    if (groove_found)
    {
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
        osDelay(10);

        for (uint32_t i = 0; i < 115; i++)
        {
            Stepper_Step();
            osDelay(delay_fast);
        }


        /* =====================================================
         * SECOND APPROACH
         * Move TOWARDS groove again, MUCH SLOWER
         * ===================================================== */

        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
        osDelay(10);

        steps_moved = 0;
        groove_found = 0;

        while (steps_moved < max_steps)
        {
            Stepper_Step();
            steps_moved++;

            /* Check groove again */
            if (Stepper_IsGrooveDetected())
            {
                groove_found = 1;
                break;
            }

            /* Much slower approach */
            osDelay(delay_slow);
        }


        /* =====================================================
         * SECOND BACK OFF
         * Move OPPOSITE direction 20 steps
         * ===================================================== */

        if (groove_found)
        {
            HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
            osDelay(10);

            for (uint32_t i = 0; i < 115; i++)
            {
                Stepper_Step();
                osDelay(delay_slow);
            }
        }
    }

    /* =========================================================
     * FINISH
     * ========================================================= */

    Stepper_Disable();
}

/**
  * @brief  Align for White LED measurement
  * @retval None
  */
void Stepper_White_LED_Align(void)
{
    uint32_t delay_fast = 20;       // First approach speed
    uint32_t delay_slow = 40;      // Second approach - much slower

    uint32_t max_steps = 10000;

    uint32_t steps_moved = 0;
    uint8_t groove_found = 0;

    /* =========================================================
     * FIRST APPROACH
     * Move TOWARDS groove
     * ========================================================= */

    Stepper_Enable();
    osDelay(10);

    /* Direction towards groove */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
    osDelay(10);

    steps_moved = 0;
    groove_found = 0;

    while (steps_moved < max_steps)
    {
        Stepper_Step();
        steps_moved++;

        /* Check groove */
        if (Stepper_IsGrooveDetected())
        {
            groove_found = 1;
            break;
        }

        osDelay(delay_fast);
    }


    /* =========================================================
     * GROOVE FOUND - FIRST BACK OFF
     * Move OPPOSITE direction 20 steps
     * ========================================================= */

    if (groove_found)
    {
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
        osDelay(10);

        for (uint32_t i = 0; i < 20; i++)
        {
            Stepper_Step();
            osDelay(delay_fast);
        }


        /* =====================================================
         * SECOND APPROACH
         * Move TOWARDS groove again, MUCH SLOWER
         * ===================================================== */

        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
        osDelay(10);

        steps_moved = 0;
        groove_found = 0;

        while (steps_moved < max_steps)
        {
            Stepper_Step();
            steps_moved++;

            /* Check groove again */
            if (Stepper_IsGrooveDetected())
            {
                groove_found = 1;
                break;
            }

            /* Much slower approach */
            osDelay(delay_slow);
        }


        /* =====================================================
         * SECOND BACK OFF
         * Move OPPOSITE direction 20 steps
         * ===================================================== */

        if (groove_found)
        {
            HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
            osDelay(10);

            for (uint32_t i = 0; i < 20; i++)
            {
                Stepper_Step();
                osDelay(delay_slow);
            }
        }
    }

    /* =========================================================
     * FINISH
     * ========================================================= */

    Stepper_Disable();
}

/*===========================================================================
 * Sequence Functions
 *===========================================================================*/

/**
  * @brief  Groove detection sequence
  * @retval None
  */
void Stepper_GrooveSequence(void)
{
    if(Stepper_IsGrooveDetected())
    {
        Stepper_Enable();
        osDelay(1);

        // Move forward 1000 steps at 5ms delay
        Stepper_MoveForward(1000, 5);

        osDelay(1000);

        // Move backward 1000 steps at 5ms delay
        Stepper_MoveReverse(1000, 5);

        Stepper_Disable();

        while(Stepper_IsGrooveDetected());
    }
}
