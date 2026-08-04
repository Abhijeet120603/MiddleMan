#include "stepper.h"

static TIM_HandleTypeDef *stepper_htim;
static uint32_t stop_time = 0;
static uint8_t running = 0;

void Stepper_Init(TIM_HandleTypeDef *htim)
{
    stepper_htim = htim;
}

//void Stepper_Start(uint8_t dir, uint32_t duration, uint32_t speed)
//{
//    // 1. Enable driver
//    HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_RESET);
//    HAL_Delay(1);   // let driver wake up
//
//    // 2. Set direction
//    HAL_GPIO_WritePin(STEPPER_DIR_PORT, STEPPER_DIR_PIN,
//                      dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
//    HAL_Delay(1);   // DIR setup time (very safe)
//
//    // 3. Configure timer
//    uint32_t arr = (1000000 / speed) - 1;
//    __HAL_TIM_SET_AUTORELOAD(stepper_htim, arr);
//    __HAL_TIM_SET_COMPARE(stepper_htim, STEPPER_TIM_CHANNEL, arr / 2);
//
//    // 4. Start stepping
//    HAL_TIM_PWM_Start(stepper_htim, STEPPER_TIM_CHANNEL);
//
//    stop_time = HAL_GetTick() + (duration * 1000);
//    running = 1;
//}


void Stepper_Start(uint8_t dir, uint32_t duration, uint32_t speed)
{
    HAL_GPIO_WritePin(STEPPER_DIR_PORT, STEPPER_DIR_PIN,
                      dir ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_Delay(2);  // DIR setup time

    HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_RESET);

    uint32_t arr = (1000000 / speed) - 1;
    __HAL_TIM_SET_AUTORELOAD(stepper_htim, arr);
    __HAL_TIM_SET_COMPARE(stepper_htim, STEPPER_TIM_CHANNEL, arr / 2);

    HAL_TIM_PWM_Start(stepper_htim, STEPPER_TIM_CHANNEL);

    stop_time = HAL_GetTick() + (duration * 1000);
    running = 1;
}


void Stepper_Update(void)
{
    if (running && HAL_GetTick() >= stop_time)
    {
        HAL_TIM_PWM_Stop(stepper_htim, STEPPER_TIM_CHANNEL);
//        HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_SET);
        running = 0;
    }
}






//#include "stepper.h"
//
//static TIM_HandleTypeDef *stepper_htim;
//
//static uint32_t stop_time_ms = 0;
//static uint8_t running = 0;
//
//void Stepper_Init(TIM_HandleTypeDef *htim)
//{
//    stepper_htim = htim;
//}
//
//static void Stepper_Enable(void)
//{
//    HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_RESET);
//}
//
//static void Stepper_Disable(void)
//{
//    HAL_GPIO_WritePin(STEPPER_EN_PORT, STEPPER_EN_PIN, GPIO_PIN_SET);
//}
//
//static void Stepper_SetDirection(uint8_t dir)
//{
//    HAL_GPIO_WritePin(
//        STEPPER_DIR_PORT,
//        STEPPER_DIR_PIN,
//        dir ? GPIO_PIN_SET : GPIO_PIN_RESET
//    );
//}
//
//static void Stepper_SetSpeed(uint32_t hz)
//{
//    if (hz == 0) return;
//
//    uint32_t arr = (1000000 / hz) - 1;
//    __HAL_TIM_SET_AUTORELOAD(stepper_htim, arr);
//    __HAL_TIM_SET_COMPARE(stepper_htim, STEPPER_TIM_CHANNEL, arr / 2);
//}
//
//void Stepper_Start(uint8_t direction, uint32_t duration_sec, uint32_t speed_hz)
//{
//    Stepper_SetDirection(direction);
//    Stepper_SetSpeed(speed_hz);
//    Stepper_Enable();
//
//    HAL_TIM_PWM_Start(stepper_htim, STEPPER_TIM_CHANNEL);
//
//    stop_time_ms = HAL_GetTick() + (duration_sec * 1000);
//    running = 1;
//}
//
//void Stepper_Stop(void)
//{
//    HAL_TIM_PWM_Stop(stepper_htim, STEPPER_TIM_CHANNEL);
//    Stepper_Disable();
//    running = 0;
//}
//
//void Stepper_Update(void)
//{
//    if (running && HAL_GetTick() >= stop_time_ms)
//    {
//        Stepper_Stop();
//    }
//}
//
