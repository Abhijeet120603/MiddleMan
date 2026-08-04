#include "stepper.h"
#include "cmsis_os.h"
/* External References */
extern TIM_HandleTypeDef htim2;

/* Private Variables */
volatile uint8_t GrooveSensorState = 0;
volatile HAL_StatusTypeDef StepperPWMStatus = HAL_OK;

static volatile uint32_t StepCount = 0;
static volatile uint32_t TargetSteps = 0;
static volatile uint8_t MotorRunning = 0;

/* Private Function Prototypes */
static void Stepper_Start(uint32_t steps);

static void Stepper_ConfigureTimer(uint32_t step_frequency);

static uint8_t Stepper_MoveUntilGroove(StepperDirection_t dir, uint32_t step_frequency, uint32_t max_steps);
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

/**
  * @brief  Set microstepping mode
  * @param  mode: MicrostepMode_t - selected microstepping mode
  * @retval None
  */
void Stepper_SetMicrostepping(MicrostepMode_t mode)
{
    // MS1 and MS2 control microstepping on most drivers
    // MS2 | MS1 | Microstepping
    //  L  |  L  | Full step
    //  L  |  H  | Half step
    //  H  |  L  | Quarter step (A4988) / 1/8 (DRV8825)
    //  H  |  H  | 1/16 (A4988) / 1/32 (DRV8825)

    GPIO_PinState ms1_state;
    GPIO_PinState ms2_state;

    switch(mode) {
        case MICROSTEP_FULL:
            ms1_state = GPIO_PIN_RESET;
            ms2_state = GPIO_PIN_RESET;
            break;
        case MICROSTEP_HALF:
            ms1_state = GPIO_PIN_SET;
            ms2_state = GPIO_PIN_RESET;
            break;
        case MICROSTEP_QUARTER:
            ms1_state = GPIO_PIN_RESET;
            ms2_state = GPIO_PIN_SET;
            break;
        case MICROSTEP_EIGHTH:
            ms1_state = GPIO_PIN_SET;
            ms2_state = GPIO_PIN_SET;
            break;
        case MICROSTEP_SIXTEENTH:
        default:
            ms1_state = GPIO_PIN_SET;
            ms2_state = GPIO_PIN_SET;
            break;
    }

    HAL_GPIO_WritePin(MS1_GPIO_Port, MS1_Pin, ms1_state);
    HAL_GPIO_WritePin(MS2_GPIO_Port, MS2_Pin, ms2_state);
}


static void Stepper_ConfigureTimer(uint32_t step_frequency)
{
    uint32_t timer_clk = 16000000;      // HSI = 16 MHz
    uint32_t prescaler = 15;            // Timer clock = 1 MHz
    uint32_t timer_freq = timer_clk / (prescaler + 1);
    uint32_t arr = (timer_freq / step_frequency) - 1;

    if(arr > 0xFFFF)
        arr = 0xFFFF;

    __HAL_TIM_SET_PRESCALER(&htim2, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (arr + 1)/2); // 50% duty

    HAL_TIM_GenerateEvent(&htim2, TIM_EVENTSOURCE_UPDATE);
}

/*===========================================================================
 * Movement Functions - Direct Control (Time Based)
 *===========================================================================*/

/**
  * @brief  Move stepper motor forward for specified duration at specified frequency
  * @param  duration_ms: Duration to run the motor in milliseconds
  * @param  step_frequency: Step frequency in Hz
  * @retval None
  */
void Stepper_MoveForward(uint32_t duration_ms, uint32_t step_frequency)
{
    Stepper_Move(POSITIVE, duration_ms, step_frequency);
}

/**
  * @brief  Move stepper motor reverse for specified duration at specified frequency
  * @param  duration_ms: Duration to run the motor in milliseconds
  * @param  step_frequency: Step frequency in Hz
  * @retval None
  */
void Stepper_MoveReverse(uint32_t duration_ms, uint32_t step_frequency)
{
    Stepper_Move(NEGATIVE, duration_ms, step_frequency);
}

/**
  * @brief  Move stepper motor in specified direction for duration at frequency
  * @param  dir: Direction to move (POSITIVE or NEGATIVE)
  * @param  duration_ms: Duration to run the motor in milliseconds
  * @param  step_frequency: Step frequency in Hz
  * @retval None
  */
void Stepper_Move(StepperDirection_t dir, uint32_t duration_ms, uint32_t step_frequency)
{
    // Configure timer for desired frequency
    Stepper_ConfigureTimer(step_frequency);

    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, (GPIO_PinState)dir);

    osDelay(1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    osDelay(duration_ms);

    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
}


/*===========================================================================
 * Movement Functions - Step Based (Interrupt Driven)
 *===========================================================================*/

/**
  * @brief  Private function to start step-based movement
  * @param  steps: Number of steps to move
  * @retval None
  */
static void Stepper_Start(uint32_t steps)
{
    StepCount = 0;
    TargetSteps = steps;
    MotorRunning = 1;

    StepperPWMStatus = HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1);

    if(StepperPWMStatus!=HAL_OK)
    {
        MotorRunning=0;
    }
}

/**
  * @brief  Move motor positive direction by specified number of steps
  * @param  steps: Number of steps to move
  * @retval None
  */
void Stepper_MovePositive(uint32_t steps, uint32_t step_frequency)
{
    // Configure timer for desired frequency
    Stepper_ConfigureTimer(step_frequency);

    // Set direction
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);

    // Start stepping with interrupt
    Stepper_Start(steps);

    // Wait for completion with timeout
    uint32_t start = osKernelGetTickCount();

    while(MotorRunning)
    {
        // Timeout after 5 seconds (safety)
        if(osKernelGetTickCount() - start > 5000)
        {
            // Force stop
            HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_1);
            MotorRunning = 0;
            break;
        }

        osDelay(1);
    }
}



/**
  * @brief  Move motor negative direction by specified number of steps at specified frequency
  * @param  steps: Number of steps to move
  * @param  step_frequency: Step frequency in Hz
  * @retval None
  */
void Stepper_MoveNegative(uint32_t steps, uint32_t step_frequency)
{
    // Configure timer for desired frequency
    Stepper_ConfigureTimer(step_frequency);

    // Set direction
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

    // Start stepping with interrupt
    Stepper_Start(steps);

    // Wait for completion with timeout
    uint32_t start = osKernelGetTickCount();

    while(MotorRunning)
    {
        // Timeout after 5 seconds (safety)
        if(osKernelGetTickCount() - start > 5000)
        {
            // Force stop
            HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_1);
            MotorRunning = 0;
            break;
        }

        osDelay(1);
    }
}

static uint8_t Stepper_MoveUntilGroove(StepperDirection_t dir, uint32_t step_frequency, uint32_t max_steps)
{
    uint32_t steps_moved = 0;

    // Enable driver
    Stepper_Enable();
    osDelay(10);

    // Configure timer for desired frequency
    Stepper_ConfigureTimer(step_frequency);

    // Set direction
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, (GPIO_PinState)dir);

    // Start PWM without interrupt (continuous movement)
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    // Move until groove detected or max steps reached
    while(steps_moved < max_steps)
    {
        // Check if groove is detected
        if(Stepper_IsGrooveDetected())
        {
            // Stop immediately
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
            Stepper_Disable();
            return 1;  // Groove detected
        }

        // Small delay to simulate step counting
        osDelay(1);
        steps_moved++;

        // Safety check - every 100 steps, verify we're still moving
        if(steps_moved % 100 == 0)
        {
            // Optional: Add heartbeat or status indication
        }
    }

    // Max steps reached without detecting groove
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    Stepper_Disable();
    return 0;  // Groove not detected
}

/*===========================================================================
 * Alignment and Homing Functions
 *===========================================================================*/

/**
  * @brief  Home the stepper motor
  *         Moves in negative direction until groove sensor is detected,
  *         then moves positive for 500 steps
  * @retval None
  */
void Stepper_Home(void)
{
    uint32_t step_frequency = 500;  // Use 500 Hz for homing

    // Enable driver
    Stepper_Enable();
    osDelay(10);

    // Move negative until groove sensor is detected
    // Max 5000 steps as safety limit
    uint8_t groove_found = Stepper_MoveUntilGroove(NEGATIVE, step_frequency, 5000);

    if(groove_found)
    {
        // Groove detected - move positive for 500 steps
        osDelay(100);  // Small delay to settle
        Stepper_MovePositive(500, step_frequency);
    }
    else
    {
        // Groove not found - move to safe position (move positive 1000 steps)
        Stepper_MovePositive(1000, step_frequency);
    }

    // Disable driver
    Stepper_Disable();
}

/**
  * @brief  Align for UV sensor measurement
  *         First homes the motor, then moves forward 100 steps
  * @retval None
  */
void Stepper_UV_Sensor_Align(void)
{
    // First home the motor
    Stepper_Home();

    // Small delay after homing
    osDelay(100);

    // Enable driver
    Stepper_Enable();
    osDelay(10);

    // Move forward 100 steps at 500 Hz
    Stepper_MovePositive(100, 500);

    // Disable driver
    Stepper_Disable();
}

/**
  * @brief  Align for White LED measurement
  *         First homes the motor, then moves forward 200 steps
  * @retval None
  */
void Stepper_White_LED_Align(void)
{
    // First home the motor
    Stepper_Home();

    // Small delay after homing
    osDelay(100);

    // Enable driver
    Stepper_Enable();
    osDelay(10);

    // Move forward 200 steps at 500 Hz
    Stepper_MovePositive(200, 500);

    // Disable driver
    Stepper_Disable();
}
/*===========================================================================
 * Callback Functions
 *===========================================================================*/

/**
  * @brief  Timer interrupt callback for step counting
  * @param  htim: Timer handle
  * @retval None
  */
void Stepper_TimerPulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance != TIM2)
        return;

    StepCount++;

    if(StepCount >= TargetSteps)
    {
        HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_1);

        MotorRunning = 0;
    }
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

        // Move forward 1000 steps at 500 Hz
        Stepper_MovePositive(1000, 500);

        osDelay(1000);

        // Move backward 1000 steps at 500 Hz
        Stepper_MoveNegative(1000, 500);

        Stepper_Disable();

        while(Stepper_IsGrooveDetected());
    }
}

