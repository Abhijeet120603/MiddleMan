#include "pump.h"


/* -------------------------------------------------------------------------- */
/* Private defines                                                            */
/* -------------------------------------------------------------------------- */

/*
 * TIM3 Period from CubeMX:
 *
 * htim3.Init.Period = 799
 *
 * Therefore the timer counts:
 *
 * 0 ... 799
 *
 * Total = 800 counts
 */
#define PUMP_TIMER_PERIOD          799U


/*
 * PWM limits
 */
#define PWM_MAX_PERCENTAGE         100U
#define PWM_MIN_PERCENTAGE         0U


/*
 * --------------------------------------------------------------------------
 * STARTUP BOOST
 * --------------------------------------------------------------------------
 *
 * Your pump does not reliably start below approximately 50%.
 *
 * Therefore:
 *
 * Requested PWM < 50%
 *
 *       ↓
 *
 * Start at 50%
 *
 *       ↓
 *
 * Wait 150 ms
 *
 *       ↓
 *
 * Drop to requested PWM
 *
 * Example:
 *
 * PUMP_Move(PUMP_FORWARD, 2000, 30);
 *
 * 50% for 150 ms
 *       +
 * 30% for remaining 1850 ms
 *
 * Adjust these values according to your pump.
 */

#define PUMP_STARTUP_PWM           60U
#define PUMP_STARTUP_TIME_MS       200U


/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

static TIM_HandleTypeDef *pump_timer = NULL;

static uint8_t pump_running = 0;

static uint8_t current_speed = 0;

static Pump_Direction_t current_direction = PUMP_FORWARD;


/* -------------------------------------------------------------------------- */
/* Private function prototypes                                                */
/* -------------------------------------------------------------------------- */

static void PUMP_SetPWM(uint8_t percentage);

static void PUMP_EnableOutputs(void);

static void PUMP_DisableOutputs(void);


/* -------------------------------------------------------------------------- */
/* PUMP INIT                                                                  */
/* -------------------------------------------------------------------------- */

void PUMP_Init(TIM_HandleTypeDef *htim)
{
    /*
     * Store timer handle
     */
    pump_timer = htim;

    /*
     * Reset internal state
     */
    pump_running = 0;

    current_speed = 0;

    current_direction = PUMP_FORWARD;

    /*
     * Make sure pump is stopped
     */
    PUMP_Stop();
}


/* -------------------------------------------------------------------------- */
/* PUMP MOVE                                                                  */
/* -------------------------------------------------------------------------- */

void PUMP_Move(
    Pump_Direction_t direction,
    uint32_t duration_ms,
    uint8_t pwm_percentage
)
{
    uint32_t actual_duration_ms;
    uint32_t startup_time_ms;


    /*
     * Make sure pump has been initialized
     */
    if (pump_timer == NULL)
    {
        return;
    }


    /*
     * Limit PWM to 0-100%
     */
    if (pwm_percentage > PWM_MAX_PERCENTAGE)
    {
        pwm_percentage = PWM_MAX_PERCENTAGE;
    }


    /*
     * If requested PWM is zero,
     * simply stop the pump.
     */
    if (pwm_percentage == 0)
    {
        PUMP_Stop();
        return;
    }


    /*
     * Stop any previous pump operation
     *
     * This guarantees that direction is changed
     * only after the pump has stopped.
     */
    PUMP_Stop();


    /*
     * Set direction
     */
    current_direction = direction;


    /*
     * Set requested speed
     */
    current_speed = pwm_percentage;


    /* ---------------------------------------------------------------------- */
    /* Configure direction                                                    */
    /* ---------------------------------------------------------------------- */

    if (direction == PUMP_FORWARD)
    {
        /*
         * Forward
         */

        HAL_GPIO_WritePin(
            Pump_Forward_TIM3_CH1_GPIO_Port,
            Pump_Forward_TIM3_CH1_Pin,
            GPIO_PIN_SET
        );

        HAL_GPIO_WritePin(
            Pump_Reverse_TIM3_CH1_GPIO_Port,
            Pump_Reverse_TIM3_CH1_Pin,
            GPIO_PIN_RESET
        );
    }
    else
    {
        /*
         * Reverse
         */

        HAL_GPIO_WritePin(
            Pump_Forward_TIM3_CH1_GPIO_Port,
            Pump_Forward_TIM3_CH1_Pin,
            GPIO_PIN_RESET
        );

        HAL_GPIO_WritePin(
            Pump_Reverse_TIM3_CH1_GPIO_Port,
            Pump_Reverse_TIM3_CH1_Pin,
            GPIO_PIN_SET
        );
    }


    /* ---------------------------------------------------------------------- */
    /* STARTUP BOOST                                                          */
    /* ---------------------------------------------------------------------- */

    /*
     * If requested speed is below startup threshold,
     * temporarily use startup PWM.
     */
    if (pwm_percentage < PUMP_STARTUP_PWM)
    {
        /*
         * Do not apply startup boost if the requested
         * duration is shorter than the boost time.
         */
        if (duration_ms > 0 &&
            duration_ms < PUMP_STARTUP_TIME_MS)
        {
            startup_time_ms = duration_ms;
        }
        else
        {
            startup_time_ms = PUMP_STARTUP_TIME_MS;
        }


        /*
         * Set startup PWM
         */
        PUMP_SetPWM(PUMP_STARTUP_PWM);


        /*
         * Start PWM
         */
        PUMP_EnableOutputs();

        pump_running = 1;


        /*
         * Startup boost
         */
        osDelay(startup_time_ms);


        /*
         * If a finite duration was requested,
         * subtract startup time.
         */
        if (duration_ms > 0)
        {
            if (duration_ms <= startup_time_ms)
            {
                /*
                 * Total requested duration is already complete.
                 */
                PUMP_Stop();
                return;
            }

            actual_duration_ms =
                duration_ms - startup_time_ms;
        }
        else
        {
            /*
             * Continuous operation
             */
            actual_duration_ms = 0;
        }


        /*
         * Change to requested low speed
         */
        PUMP_SetPWM(pwm_percentage);
    }
    else
    {
        /*
         * No startup boost required.
         */
        PUMP_SetPWM(pwm_percentage);

        /*
         * Entire requested duration is still remaining.
         */
        actual_duration_ms = duration_ms;
    }


    /* ---------------------------------------------------------------------- */
    /* ENABLE PUMP                                                            */
    /* ---------------------------------------------------------------------- */

    /*
     * In the startup-boost case PWM is already running.
     *
     * In the normal case it hasn't been started yet.
     */
    if (!pump_running)
    {
        PUMP_EnableOutputs();

        pump_running = 1;
    }


    /* ---------------------------------------------------------------------- */
    /* HANDLE DURATION                                                        */
    /* ---------------------------------------------------------------------- */

    if (actual_duration_ms > 0)
    {
        /*
         * Wait for requested remaining time.
         */
        osDelay(actual_duration_ms);


        /*
         * Stop pump automatically.
         */
        PUMP_Stop();
    }
}


/* -------------------------------------------------------------------------- */
/* PUMP STOP                                                                 */
/* -------------------------------------------------------------------------- */

void PUMP_Stop(void)
{
    if (pump_timer == NULL)
    {
        return;
    }


    /*
     * Disable PWM outputs
     */
    PUMP_DisableOutputs();


    /*
     * Set PWM to zero
     */
    PUMP_SetPWM(0);


    /*
     * Turn OFF both direction pins
     */
    HAL_GPIO_WritePin(
        Pump_Forward_TIM3_CH1_GPIO_Port,
        Pump_Forward_TIM3_CH1_Pin,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        Pump_Reverse_TIM3_CH1_GPIO_Port,
        Pump_Reverse_TIM3_CH1_Pin,
        GPIO_PIN_RESET
    );


    /*
     * Reset internal state
     */
    pump_running = 0;

    current_speed = 0;
}


/* -------------------------------------------------------------------------- */
/* SET SPEED                                                                  */
/* -------------------------------------------------------------------------- */

void PUMP_SetSpeed(uint8_t percentage)
{
    if (pump_timer == NULL)
    {
        return;
    }


    /*
     * Limit PWM to 0-100%
     */
    if (percentage > PWM_MAX_PERCENTAGE)
    {
        percentage = PWM_MAX_PERCENTAGE;
    }


    /*
     * Store requested speed
     */
    current_speed = percentage;


    /*
     * Only modify PWM if pump is running.
     */
    if (pump_running)
    {
        PUMP_SetPWM(percentage);
    }
}


/* -------------------------------------------------------------------------- */
/* IS RUNNING                                                                 */
/* -------------------------------------------------------------------------- */

uint8_t PUMP_IsRunning(void)
{
    return pump_running;
}


/* -------------------------------------------------------------------------- */
/* SET PWM                                                                    */
/* -------------------------------------------------------------------------- */

static void PUMP_SetPWM(uint8_t percentage)
{
    uint32_t pulse_value;


    if (pump_timer == NULL)
    {
        return;
    }


    /*
     * Limit PWM
     */
    if (percentage > PWM_MAX_PERCENTAGE)
    {
        percentage = PWM_MAX_PERCENTAGE;
    }


    /*
     * Calculate compare value.
     *
     * Timer period = 799
     *
     * Therefore:
     *
     * 100% = 800 counts
     * 50%  = 400 counts
     * 30%  = 240 counts
     * 20%  = 160 counts
     */
    pulse_value =
        ((PUMP_TIMER_PERIOD + 1U) * percentage) / 100U;


    /*
     * Safety limit.
     */
    if (pulse_value > (PUMP_TIMER_PERIOD + 1U))
    {
        pulse_value = PUMP_TIMER_PERIOD + 1U;
    }


    /*
     * Set PWM on active direction channel.
     */
    if (current_direction == PUMP_FORWARD)
    {
        /*
         * Forward = TIM3 CH1
         */
        __HAL_TIM_SET_COMPARE(
            pump_timer,
            TIM_CHANNEL_1,
            pulse_value
        );


        /*
         * Make sure reverse channel is zero.
         */
        __HAL_TIM_SET_COMPARE(
            pump_timer,
            TIM_CHANNEL_2,
            0
        );
    }
    else
    {
        /*
         * Reverse = TIM3 CH2
         */
        __HAL_TIM_SET_COMPARE(
            pump_timer,
            TIM_CHANNEL_2,
            pulse_value
        );


        /*
         * Make sure forward channel is zero.
         */
        __HAL_TIM_SET_COMPARE(
            pump_timer,
            TIM_CHANNEL_1,
            0
        );
    }
}


/* -------------------------------------------------------------------------- */
/* ENABLE OUTPUTS                                                             */
/* -------------------------------------------------------------------------- */

static void PUMP_EnableOutputs(void)
{
    if (pump_timer == NULL)
    {
        return;
    }


    /*
     * Start both PWM channels.
     *
     * Only the active channel has a non-zero compare value.
     */
    HAL_TIM_PWM_Start(
        pump_timer,
        TIM_CHANNEL_1
    );

    HAL_TIM_PWM_Start(
        pump_timer,
        TIM_CHANNEL_2
    );
}


/* -------------------------------------------------------------------------- */
/* DISABLE OUTPUTS                                                            */
/* -------------------------------------------------------------------------- */

static void PUMP_DisableOutputs(void)
{
    if (pump_timer == NULL)
    {
        return;
    }


    /*
     * Stop both PWM channels.
     */
    HAL_TIM_PWM_Stop(
        pump_timer,
        TIM_CHANNEL_1
    );

    HAL_TIM_PWM_Stop(
        pump_timer,
        TIM_CHANNEL_2
    );
}


/* -------------------------------------------------------------------------- */
/* USER CODE                                                                  */
/* -------------------------------------------------------------------------- */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
