/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ds18b20.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// Control Stages
typedef enum {
    STAGE_1_FAST_HEAT = 1,    // 100% power to 32°C
    STAGE_2_CONTROLLED,       // 60% power to setpoint-2
    STAGE_3_PID,              // Full PID control
    STAGE_ERROR               // Error state
} ControlStage_TypeDef;

// PID Structure
typedef struct {
    float kp;
    float ki;
    float kd;
    float dt;                 // Sample time in seconds
    float setpoint;
    float integral;
    float prev_error;
    float output_min;
    float output_max;
    float output;
} PID_TypeDef;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Peltier Control Pins
#define Cuvette_Peltier_Pin GPIO_PIN_12
#define Cuvette_Peltier_GPIO_Port GPIOB

// PID Constants (Start with conservative values and tune based on response)
#define KP 30.0f    // Start lower to avoid overshoot
#define KI 1.0f     // Conservative integral
#define KD 5.0f     // Conservative derivative (reduced from 8.0)

// Control Parameters
#define CONTROL_INTERVAL_MS 800U      // DS18B20 read interval
#define STAGE_1_TARGET 32.0f         // Temperature to switch from Stage 1 to 2
#define SENSOR_MAX_ATTEMPTS 5U        // Retry attempts for sensor detection
#define STABILITY_SAMPLES 8          // Number of samples for stability check (6.4 seconds)
#define STABILITY_THRESHOLD 0.1f     // °C range for stability

// Software PWM Parameters
#define PWM_PERIOD_MS 100
#define PWM_PERIOD_STEPS 100U    // 100 x 1ms = 100ms = 10Hz
                                // 1% duty-cycle resolution

// UART Buffer Size
#define UART_BUFFER_SIZE 128
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart6_rx;
DMA_HandleTypeDef hdma_usart6_tx;

/* USER CODE BEGIN PV */
// Global temperature variables
extern uint8_t temp_sens_pres;     // 0: sensor not found, 1: sensor found
extern uint8_t temp_out_of_range;  // 1: temp > 40°C, 0: temp <= 40°C
extern float Temperature;          // Current temperature from DS18B20

// Control variables
static ControlStage_TypeDef current_stage = STAGE_1_FAST_HEAT;
static PID_TypeDef pid_controller;
static float target_temperature = 37.0f;
static float stage_2_target = 0.0f;
static uint32_t control_timer = 0;
static uint32_t start_time_ms = 0;
static uint32_t elapsed_time_ms = 0;

// Stage tracking
static bool stage_1_reached = false;
static bool stage_2_reached = false;

// Stability detection
static float temp_history[STABILITY_SAMPLES];
static uint8_t history_index = 0;
static uint8_t history_count = 0;

// Sensor status

static bool sensor_ok = false;
static bool sensor_was_lost = false;

// UART buffer
static char uart_buffer[UART_BUFFER_SIZE];

// PWM variables (used in interrupt - must be volatile)
static volatile uint8_t peltier_power = 0;    // 0-100%
static volatile uint8_t pwm_counter = 0;

volatile uint32_t current_time = 0;
volatile uint32_t debug_counter = 0;
volatile uint32_t counter_value = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
// Function prototypes
void Peltier_SetPower(uint8_t power_percent);
void Peltier_Off(void);
bool Peltier_CheckSensor(void);
void PID_Init(PID_TypeDef *pid, float kp, float ki, float kd, float dt, float min, float max);
float PID_Compute(PID_TypeDef *pid, float input, float setpoint);
ControlStage_TypeDef Peltier_UpdateControl(float set_temp);
bool IsTemperatureStable(void);
void UART_SendStatus(void);
void UART_Init(void);
void UART_SendString(char *str);

void Test_Timer_Interrupt(void);

void Peltier_PWM_Update_SysTick(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Set Peltier power (0-100%)
// This just sets the variable - TIM10 interrupt handles the actual PWM
void Peltier_SetPower(uint8_t power_percent) {
    if (power_percent > 100) power_percent = 100;
    peltier_power = power_percent;
}



// Check if sensor is present with retries (for initial startup only)
bool Peltier_CheckSensor(void) {
    uint8_t attempts = 0;

    while (attempts < SENSOR_MAX_ATTEMPTS) {
        DS18B20_GetTemperature();
        HAL_Delay(2000);  // 2 second delay between attempts

        if (temp_sens_pres == 1) {
            sensor_ok = true;
            return true;
        }
        attempts++;
    }

    sensor_ok = false;
    Peltier_Off();
    // DO NOT change current_stage - preserve it
    return false;
}

// Initialize PID controller
void PID_Init(PID_TypeDef *pid, float kp, float ki, float kd, float dt, float min, float max) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->dt = dt;
    pid->setpoint = 0.0f;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output_min = min;
    pid->output_max = max;
    pid->output = 0.0f;
}

// Compute PID output with proper dt
float PID_Compute(PID_TypeDef *pid, float input, float setpoint) {
    pid->setpoint = setpoint;
    float error = setpoint - input;

    // Proportional term
    float proportional = pid->kp * error;

    // Integral term with anti-windup
    pid->integral += error * pid->dt;

    // Clamp integral to prevent windup
    float integral_limit = 100.0f / pid->ki;
    if (pid->integral > integral_limit) pid->integral = integral_limit;
    if (pid->integral < -integral_limit) pid->integral = -integral_limit;

    float integral = pid->ki * pid->integral;

    // Derivative term with dt
    float derivative = pid->kd * (error - pid->prev_error) / pid->dt;
    pid->prev_error = error;

    // Calculate output
    float output = proportional + integral + derivative;

    // Clamp output
    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;

    pid->output = output;
    return output;
}

// Check if temperature is stable (within threshold over sample period)
bool IsTemperatureStable(void) {
    if (history_count < STABILITY_SAMPLES) {
        return false;
    }

    float min_temp = temp_history[0];
    float max_temp = temp_history[0];

    for (uint8_t i = 1; i < STABILITY_SAMPLES; i++) {
        if (temp_history[i] < min_temp) min_temp = temp_history[i];
        if (temp_history[i] > max_temp) max_temp = temp_history[i];
    }

    return ((max_temp - min_temp) <= STABILITY_THRESHOLD);
}

ControlStage_TypeDef Peltier_UpdateControl(float set_temp)
{
    /* ============================================================
     * SENSOR CHECK
     * ============================================================ */

    if (temp_sens_pres == 0)
    {
        // Sensor lost -> immediately turn Peltier OFF
        Peltier_Off();

        sensor_ok = false;

        // IMPORTANT:
        // Do NOT change current_stage.
        // The same stage will be resumed when sensor returns.
        return current_stage;
    }

    /* ============================================================
     * SENSOR IS PRESENT
     * ============================================================ */

    sensor_ok = true;

    /* ============================================================
     * SAFETY TEMPERATURE CHECK
     * ============================================================ */

    if (temp_out_of_range || Temperature > 40.0f)
    {
        Peltier_Off();

        current_stage = STAGE_ERROR;

        return STAGE_ERROR;
    }

    /* ============================================================
     * UPDATE TARGETS
     * ============================================================ */

    target_temperature = set_temp;

    stage_2_target = set_temp - 2.0f;

    /* ============================================================
     * UPDATE TEMPERATURE HISTORY
     * ============================================================ */

    temp_history[history_index] = Temperature;

    history_index =
        (history_index + 1) % STABILITY_SAMPLES;

    if (history_count < STABILITY_SAMPLES)
    {
        history_count++;
    }

    /* ============================================================
     * STAGE CONTROL
     * ============================================================ */

    switch (current_stage)
    {
        /* ========================================================
         * STAGE 1
         * ======================================================== */

        case STAGE_1_FAST_HEAT:

            /*
             * Sensor is present.
             *
             * If sensor was previously lost while in Stage 1,
             * this automatically restores 100% power.
             */

            Peltier_SetPower(100);

            /* Check if 32°C has been reached */
            if (Temperature >= STAGE_1_TARGET)
            {
                stage_1_reached = true;

                current_stage = STAGE_2_CONTROLLED;
            }

            break;


        /* ========================================================
         * STAGE 2
         * ======================================================== */

        case STAGE_2_CONTROLLED:

            /*
             * Sensor recovered while in Stage 2:
             *
             * Temperature < setpoint-2 -> 60%
             *
             * Temperature >= setpoint-2 -> 40%
             */

            if (Temperature < stage_2_target)
            {
                Peltier_SetPower(60);
            }
            else
            {
                stage_2_reached = true;

                /*
                 * Reduced power while waiting for stabilization
                 */
                Peltier_SetPower(40);

                /*
                 * Once stable, move to PID
                 */
                if (IsTemperatureStable())
                {
                    current_stage = STAGE_3_PID;

                    /*
                     * Clear PID integral
                     */
                    pid_controller.integral = 0.0f;

                    /*
                     * IMPORTANT:
                     * Set previous error to the CURRENT error.
                     *
                     * This prevents a large derivative spike
                     * immediately after sensor recovery.
                     */
                    pid_controller.prev_error =
                        target_temperature - Temperature;
                }
            }

            break;


        /* ========================================================
         * STAGE 3 - PID
         * ======================================================== */

        case STAGE_3_PID:
        {
            float pid_output;

            /*
             * Calculate PID output using the newly recovered
             * temperature.
             */
            pid_output =
                PID_Compute(
                    &pid_controller,
                    Temperature,
                    target_temperature
                );

            /* Limit output to 0-100% */
            if (pid_output > 100.0f)
            {
                pid_output = 100.0f;
            }

            if (pid_output < 0.0f)
            {
                pid_output = 0.0f;
            }

            /*
             * This restores the actual PWM power.
             *
             * TIM10 is already running, so we only need to
             * update peltier_power here.
             */
            Peltier_SetPower((uint8_t)pid_output);

            /* Safety check */
            if (temp_out_of_range || Temperature > 40.0f)
            {
                Peltier_Off();

                current_stage = STAGE_ERROR;
            }

            break;
        }


        /* ========================================================
         * ERROR STATE
         * ======================================================== */

        case STAGE_ERROR:

            /*
             * Permanent safety/error condition.
             * Peltier remains OFF.
             */
            Peltier_Off();

            break;


        /* ========================================================
         * INVALID STAGE
         * ======================================================== */

        default:

            Peltier_Off();

            current_stage = STAGE_ERROR;

            break;
    }

    return current_stage;
}

// UART Functions
void UART_Init(void) {
    char init_msg[] = "Cuvette Temperature Control System Started\r\n";
    UART_SendString(init_msg);
}

void UART_SendString(char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

void UART_SendStatus(void) {
    float error = target_temperature - Temperature;
    float actual_time_sec = elapsed_time_ms / 1000.0f;

    // Format: TIME(sec),STAGE,TEMP,SETPOINT,POWER,ERROR
    snprintf(uart_buffer, UART_BUFFER_SIZE, "%.1f,%d,%.2f,%.2f,%d,%.2f\r\n",
             (double)actual_time_sec,
             current_stage,
             (double)Temperature,
             (double)target_temperature,
             peltier_power,
             (double)error);

    UART_SendString(uart_buffer);
}

void Peltier_Off(void)
{
    /*
     * Immediately disable software PWM
     */
    peltier_power = 0;

    /*
     * Immediately force physical output LOW
     */
    HAL_GPIO_WritePin(
        Cuvette_Peltier_GPIO_Port,
        Cuvette_Peltier_Pin,
        GPIO_PIN_RESET
    );
}

void Peltier_PWM_Update_SysTick(void)
{
    static uint32_t last_pwm_time = 0;
    static uint8_t pwm_count = 0;

    uint32_t current_ms = HAL_GetTick();

    // Update every 1ms using SysTick
    if (current_ms - last_pwm_time >= 1)
    {
        last_pwm_time = current_ms;

        pwm_count++;
        if (pwm_count >= PWM_PERIOD_STEPS)
        {
            pwm_count = 0;
        }

        // Generate PWM on PB12
        if (pwm_count < peltier_power)
        {
            HAL_GPIO_WritePin(Cuvette_Peltier_GPIO_Port, Cuvette_Peltier_Pin, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(Cuvette_Peltier_GPIO_Port, Cuvette_Peltier_Pin, GPIO_PIN_RESET);
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART6_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

//  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
//  {
//      Error_Handler();
//  }
//
//
//  // Check if timer is running
//  uint32_t counter_value = __HAL_TIM_GET_COUNTER(&htim2);
//  char debug_msg[64];
//  snprintf(debug_msg, sizeof(debug_msg), "TIM2 Counter: %lu\r\n", counter_value);
//  UART_SendString(debug_msg);

  // Initialize UART
  UART_Init();

  // Start TIM10 interrupt for PWM generation on PB12
  // PB12 is configured as push-pull output in MX_GPIO_Init()
  // TIM10 interrupt will toggle PB12 at 10Hz with 1% resolution
//  HAL_TIM_Base_Start_IT(&htim10);

  // Initialize PID controller with 1 second sampling time
  PID_Init(&pid_controller, KP, KI, KD, 1.0f, 0.0f, 100.0f);

  // Check if sensor is present
  if (!Peltier_CheckSensor()) {
      UART_SendString("Sensor not found. Peltier OFF. Waiting for sensor...\r\n");
      Peltier_Off();
      // IMPORTANT: Do NOT enter STAGE_ERROR.
      // Keep the initial stage.
      current_stage = STAGE_1_FAST_HEAT;
  } else {
      UART_SendString("Sensor found. Starting control loop.\r\n");
  }

  // Set target temperature (you can change this value)
  target_temperature = 37.0f;
  UART_SendString("Target temperature set to 37.0°C\r\n");

  // Reset control state
  current_stage = STAGE_1_FAST_HEAT;
  stage_1_reached = false;
  stage_2_reached = false;
  start_time_ms = HAL_GetTick();
  elapsed_time_ms = 0;
  control_timer = HAL_GetTick();

  // Send CSV header
  UART_SendString("TIME(sec),STAGE,TEMP,SETPOINT,POWER,ERROR\r\n");

//  Test_Timer_Interrupt();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

//	    current_time = HAL_GetTick();
//
////	    uint32_t current_time = HAL_GetTick();
//
//      if (current_time - control_timer >= CONTROL_INTERVAL_MS)
//      {
//          control_timer = current_time;
//
//          elapsed_time_ms =
//              current_time - start_time_ms;
//
//          /*
//           * Read sensor ONCE
//           */
//          DS18B20_GetTemperature();
//
//
//          /* =========================================================
//           * SENSOR VALUE BASED SAFETY
//           *
//           * Temperature <= 5°C  -> SENSOR LOST
//           * Temperature > 5°C   -> SENSOR FOUND
//           * ========================================================= */
//
//          if (Temperature <= 5.0f)
//          {
//              /* =====================================================
//               * SENSOR LOST
//               * ===================================================== */
//
//              temp_sens_pres = 0;
//
//              /* Immediately force power to ZERO */
//              peltier_power = 0;
//
//              /* Immediately force PB12 LOW */
//              HAL_GPIO_WritePin(
//                  Cuvette_Peltier_GPIO_Port,
//                  Cuvette_Peltier_Pin,
//                  GPIO_PIN_RESET
//              );
//
//              sensor_ok = false;
//
//              if (sensor_was_lost == false)
//              {
//                  UART_SendString(
//                      "SENSOR LOST -> POWER = 0\r\n"
//                  );
//
//                  sensor_was_lost = true;
//              }
//          }
//          else
//          {
//              /* =====================================================
//               * SENSOR FOUND
//               * ===================================================== */
//
//              temp_sens_pres = 1;
//              sensor_ok = true;
//
//              if (sensor_was_lost == true)
//              {
//                  sensor_was_lost = false;
//
//                  UART_SendString(
//                      "SENSOR FOUND -> RESTORING POWER\r\n"
//                  );
//              }
//
//              /* =====================================================
//               * RESTORE PREVIOUS CONTROL STAGE
//               * ===================================================== */
//
//              if (current_stage != STAGE_ERROR)
//              {
//                  Peltier_UpdateControl(target_temperature);
//              }
//          }
//
//          UART_SendStatus();
//
//          if (debug_counter > 0)
//          {
//              char dbg_msg[64];
//              snprintf(dbg_msg, sizeof(dbg_msg), "Debug Counter: %lu\r\n", debug_counter);
//              UART_SendString(dbg_msg);
//              debug_counter = 0;  // Reset to see if it increments again
//          }
//      }
//
//
//      // Small delay to prevent busy loop
//      HAL_Delay(1);


	    // Update PWM using SysTick (1ms resolution)
	    Peltier_PWM_Update_SysTick();

	    current_time = HAL_GetTick();

	    if (current_time - control_timer >= CONTROL_INTERVAL_MS)
	    {
	        control_timer = current_time;
	        elapsed_time_ms = current_time - start_time_ms;

	        // Read sensor ONCE
	        DS18B20_GetTemperature();

	        /* SENSOR VALUE BASED SAFETY */
	        if (Temperature <= 5.0f)
	        {
	            /* SENSOR LOST */
	            temp_sens_pres = 0;
	            peltier_power = 0;
	            HAL_GPIO_WritePin(Cuvette_Peltier_GPIO_Port, Cuvette_Peltier_Pin, GPIO_PIN_RESET);
	            sensor_ok = false;

	            if (sensor_was_lost == false)
	            {
	                UART_SendString("SENSOR LOST -> POWER = 0\r\n");
	                sensor_was_lost = true;
	            }
	        }
	        else
	        {
	            /* SENSOR FOUND */
	            temp_sens_pres = 1;
	            sensor_ok = true;

	            if (sensor_was_lost == true)
	            {
	                sensor_was_lost = false;
	                UART_SendString("SENSOR FOUND -> RESTORING POWER\r\n");
	            }

	            if (current_stage != STAGE_ERROR)
	            {
	                Peltier_UpdateControl(target_temperature);
	            }
	        }

	        UART_SendStatus();
	    }

	    // Small delay to prevent busy loop
	    HAL_Delay(1);


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
//  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
//  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
//  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
//  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }
  /* USER CODE BEGIN TIM2_Init 2 */

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_HalfDuplex_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, UV_Peltier_Pin|Cuvette_Peltier_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : UV_Peltier_Pin Cuvette_Peltier_Pin */
  GPIO_InitStruct.Pin = UV_Peltier_Pin|Cuvette_Peltier_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief TIM10 Period Elapsed Callback
  * This runs every 1ms and generates PWM on PB12
  *
  * PWM Period = 100ms (10Hz)
  * Resolution = 1% (100 steps)
  */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM10)
//    {
//        // Increment counter (0-99 for 100ms period)
//        pwm_counter++;
//        if (pwm_counter >= PWM_PERIOD_STEPS)
//        {
//            pwm_counter = 0;
//        }
//
//        // Generate PWM
//        // PB12 is HIGH for peltier_power ms out of 100ms
//        if (pwm_counter < peltier_power)
//        {
//            HAL_GPIO_WritePin(Cuvette_Peltier_GPIO_Port, Cuvette_Peltier_Pin, GPIO_PIN_SET);
//        }
//        else
//        {
//            HAL_GPIO_WritePin(Cuvette_Peltier_GPIO_Port, Cuvette_Peltier_Pin, GPIO_PIN_RESET);
//        }
//    }
//}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM10)
//    {
//        debug_counter++;
//
//        pwm_counter++;
//
//        if (pwm_counter >= PWM_PERIOD_STEPS)
//        {
//            pwm_counter = 0;
//        }
//
//        if (pwm_counter < peltier_power)
//        {
//            HAL_GPIO_WritePin(
//                Cuvette_Peltier_GPIO_Port,
//                Cuvette_Peltier_Pin,
//                GPIO_PIN_SET
//            );
//        }
//        else
//        {
//            HAL_GPIO_WritePin(
//                Cuvette_Peltier_GPIO_Port,
//                Cuvette_Peltier_Pin,
//                GPIO_PIN_RESET
//            );
//        }
//    }
//}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        debug_counter++;

        pwm_counter++;

        if (pwm_counter >= PWM_PERIOD_STEPS)
        {
            pwm_counter = 0;
        }

        if (pwm_counter < peltier_power)
        {
            HAL_GPIO_WritePin(
                Cuvette_Peltier_GPIO_Port,
                Cuvette_Peltier_Pin,
                GPIO_PIN_SET
            );
        }
        else
        {
            HAL_GPIO_WritePin(
                Cuvette_Peltier_GPIO_Port,
                Cuvette_Peltier_Pin,
                GPIO_PIN_RESET
            );
        }
    }
}

void Test_Timer_Interrupt(void)
{
    UART_SendString("Testing TIM2 interrupt...\r\n");

    // Set a known power level
    peltier_power = 50;

    // Start timer
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        UART_SendString("Failed to start TIM2!\r\n");
        return;
    }

    UART_SendString("TIM2 started. Wait 2 seconds...\r\n");
    HAL_Delay(2000);

    // Check if interrupt fired
    if (debug_counter > 0)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "Interrupt fired! Count: %lu\r\n", debug_counter);
        UART_SendString(msg);
    }
    else
    {
        UART_SendString("Interrupt NOT firing!\r\n");
        UART_SendString("Check: TIM2 NVIC enabled in CubeMX\r\n");
        UART_SendString("Check: TIM2_IRQHandler exists in stm32f4xx_it.c\r\n");
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
