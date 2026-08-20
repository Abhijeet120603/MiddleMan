#include "ds18b20.h"
#include "main.h"

// External UART handle - defined in main.c
extern UART_HandleTypeDef huart6;

// Temperature variables
int16_t Temp = 0;
float Temperature = 0.0f;
uint8_t temp_sens_pres = 0;  // 0: not found, 1: found
uint8_t temp_out_of_range = 0; // 0: normal, 1: out of range (>40°C)

// Private function prototypes
static void uart_init(uint32_t baud);
static int ds18b20_reset(void);

/**
 * @brief Initialize UART6 with specified baud rate
 * @param baud: Baud rate for UART communication
 */
static void uart_init(uint32_t baud)
{
    huart6.Instance = USART6;
    huart6.Init.BaudRate = baud;
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
}

/**
 * @brief DS18B20 Reset and presence check
 * @return 1 if presence detected, 0 if not
 */
static int ds18b20_reset(void)
{
    uint8_t tx_data = 0xF0;
    uint8_t rx_data = 0xFF;

    // Switch to 9600 baud for reset pulse
    uart_init(9600);

    // Send reset pulse (480us low)
    HAL_UART_Transmit(&huart6, &tx_data, 1, 100);

    // Wait for presence pulse (sensor pulls line low)
    if (HAL_UART_Receive(&huart6, &rx_data, 1, 100) != HAL_OK)
    {
        // No response or timeout
        uart_init(115200);
        return 0;
    }

    // Switch back to 115200 for data transfer
    uart_init(115200);

    // Check if sensor responded (pulled line low)
    if (rx_data == 0xF0)
    {
        return 0;  // No presence detected
    }

    return 1;  // Presence detected
}

/**
 * @brief Check if DS18B20 sensor is present with retry
 * @return 1 if sensor found, 0 if not found
 */
//int DS18B20_CheckPresence(void)
//{
//    int retry_count = 0;
//    int result = 0;
//
//    while (retry_count < 5)
//    {
//        result = ds18b20_reset();
//
//        if (result == 1)  // Sensor responded
//        {
//            temp_sens_pres = 1;
//            return 1;
//        }
//
//        retry_count++;
//        if (retry_count < 5)
//        {
//            HAL_Delay(2000);  // Wait 2 seconds before retry
//        }
//    }
//
//    // All 5 attempts failed
//    temp_sens_pres = 0;
//    return 0;
//}

int DS18B20_CheckPresence(void)
{
    int result;

    result = ds18b20_reset();

    if (result == 1)
    {
        temp_sens_pres = 1;
        return 1;
    }

    temp_sens_pres = 0;

    return 0;
}

/**
 * @brief Start communication with DS18B20 sensor (legacy - kept for compatibility)
 * @return 1 if response detected, -1 if receive failed, -2 if no response
 */
int DS18B20_Start(void)
{
    return ds18b20_reset();
}

/**
 * @brief Write a byte to DS18B20
 * @param data: Byte to write
 */
void DS18B20_Write(uint8_t data)
{
    uint8_t buffer[8];
    uint8_t tx_data;

    // Make sure we're at 115200 baud
    uart_init(115200);

    for (int i = 0; i < 8; i++)
    {
        if (data & (1 << i))
        {
            // Write '1' - send 0xFF (short low pulse)
            tx_data = 0xFF;
        }
        else
        {
            // Write '0' - send 0x00 (long low pulse)
            tx_data = 0x00;
        }

        // Transmit the bit
        HAL_UART_Transmit(&huart6, &tx_data, 1, 100);

        // Small delay between bits
        HAL_Delay(1);
    }
}

/**
 * @brief Read a byte from DS18B20
 * @return Read byte value
 */
uint8_t DS18B20_Read(void)
{
    uint8_t tx_data = 0xFF;
    uint8_t rx_data;
    uint8_t value = 0;

    // Make sure we're at 115200 baud
    uart_init(115200);

    for (int i = 0; i < 8; i++)
    {
        // Send read time slot (1us low pulse then release)
        tx_data = 0xFF;
        HAL_UART_Transmit(&huart6, &tx_data, 1, 100);

        // Small delay before reading
        HAL_Delay(1);

        // Read the bit
        if (HAL_UART_Receive(&huart6, &rx_data, 1, 10) == HAL_OK)
        {
            if (rx_data == 0xFF)
            {
                value |= (1 << i);
            }
        }

        // Delay between bits
        HAL_Delay(1);
    }

    return value;
}


//float DS18B20_GetTemperature(void)
//{
//    uint8_t Temp_LSB;
//    uint8_t Temp_MSB;
//    uint8_t Scratchpad[9];
//
//    int16_t raw_temp;
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 1: CHECK SENSOR PRESENCE
//     * ---------------------------------------------------------
//     */
//
//    if (DS18B20_CheckPresence() == 0)
//    {
//        temp_sens_pres = 0;
//        temp_out_of_range = 0;
//
//        /*
//         * Do NOT use old temperature as valid temperature.
//         */
//        Temperature = 0.0f;
//
//        return 0.0f;
//    }
//
//    /*
//     * Sensor detected
//     */
//    temp_sens_pres = 1;
//
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 2: START TEMPERATURE CONVERSION
//     * ---------------------------------------------------------
//     */
//
//    if (ds18b20_reset() == 0)
//    {
//        temp_sens_pres = 0;
//        Temperature = 0.0f;
//
//        return 0.0f;
//    }
//
//    DS18B20_Write(0xCC);       // Skip ROM
//    DS18B20_Write(0x44);       // Convert T
//
//
//    /*
//     * DS18B20 12-bit conversion
//     *
//     * Maximum conversion time = 750 ms
//     */
//    HAL_Delay(750);
//
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 3: CHECK SENSOR AGAIN
//     * ---------------------------------------------------------
//     */
//
//    if (ds18b20_reset() == 0)
//    {
//        temp_sens_pres = 0;
//        Temperature = 0.0f;
//
//        return 0.0f;
//    }
//
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 4: READ SCRATCHPAD
//     * ---------------------------------------------------------
//     */
//
//    DS18B20_Write(0xCC);       // Skip ROM
//    DS18B20_Write(0xBE);       // Read Scratchpad
//
//
//    for (int i = 0; i < 9; i++)
//    {
//        Scratchpad[i] = DS18B20_Read();
//    }
//
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 5: CHECK FOR INVALID DATA
//     * ---------------------------------------------------------
//     */
//
//    uint8_t all_ff = 1;
//
//    for (int i = 0; i < 9; i++)
//    {
//        if (Scratchpad[i] != 0xFF)
//        {
//            all_ff = 0;
//            break;
//        }
//    }
//
//    if (all_ff)
//    {
//        temp_sens_pres = 0;
//        Temperature = 0.0f;
//
//        return 0.0f;
//    }
//
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 6: CONVERT RAW TEMPERATURE
//     * ---------------------------------------------------------
//     */
//
//    Temp_LSB = Scratchpad[0];
//    Temp_MSB = Scratchpad[1];
//
//    raw_temp = (int16_t)((Temp_MSB << 8) | Temp_LSB);
//
//    Temperature = (float)raw_temp / 16.0f;
//
//
//    /*
//     * ---------------------------------------------------------
//     * STEP 7: TEMPERATURE SAFETY
//     * ---------------------------------------------------------
//     */
//
//    if (Temperature > 40.0f)
//    {
//        temp_out_of_range = 1;
//    }
//    else
//    {
//        temp_out_of_range = 0;
//    }
//
//    /*
//     * Sensor successfully read
//     */
//    temp_sens_pres = 1;
//
//    return Temperature;
//}


float DS18B20_GetTemperature(void)
{
    uint8_t Temp_LSB;
    uint8_t Temp_MSB;
    uint8_t Scratchpad[9];

    int16_t raw_temp;

    /* =========================================================
     * 1. CHECK SENSOR PRESENCE
     * ========================================================= */

    if (DS18B20_CheckPresence() == 0)
    {
        temp_sens_pres = 0;
        temp_out_of_range = 0;
        Temperature = 0.0f;

        return 0.0f;
    }

    /* Sensor found */
    temp_sens_pres = 1;


    /* =========================================================
     * 2. START CONVERSION
     * ========================================================= */

    if (ds18b20_reset() == 0)
    {
        temp_sens_pres = 0;
        temp_out_of_range = 0;
        Temperature = 0.0f;

        return 0.0f;
    }

    DS18B20_Write(0xCC);
    DS18B20_Write(0x44);

    HAL_Delay(750);


    /* =========================================================
     * 3. SENSOR MUST STILL BE PRESENT
     * ========================================================= */

    if (ds18b20_reset() == 0)
    {
        temp_sens_pres = 0;
        temp_out_of_range = 0;
        Temperature = 0.0f;

        return 0.0f;
    }


    /* =========================================================
     * 4. READ SCRATCHPAD
     * ========================================================= */

    DS18B20_Write(0xCC);
    DS18B20_Write(0xBE);

    for (int i = 0; i < 9; i++)
    {
        Scratchpad[i] = DS18B20_Read();
    }


    /* =========================================================
     * 5. VALIDATE SCRATCHPAD
     * ========================================================= */

    uint8_t all_ff = 1;

    for (int i = 0; i < 9; i++)
    {
        if (Scratchpad[i] != 0xFF)
        {
            all_ff = 0;
            break;
        }
    }

    if (all_ff)
    {
        temp_sens_pres = 0;
        temp_out_of_range = 0;
        Temperature = 0.0f;

        return 0.0f;
    }


    /* =========================================================
     * 6. VALIDATE TEMPERATURE BY RAW VALUE
     * ========================================================= */

    Temp_LSB = Scratchpad[0];
    Temp_MSB = Scratchpad[1];

    raw_temp = (int16_t)((Temp_MSB << 8) | Temp_LSB);

    /*
     * DS18B20 power-up/default scratchpad is commonly 85°C.
     * This is optional, but prevents treating an invalid
     * startup value as a real measurement.
     */

    if (raw_temp == 0x0550)
    {
        temp_sens_pres = 0;
        Temperature = 0.0f;

        return 0.0f;
    }


    /* =========================================================
     * 7. CONVERT TEMPERATURE
     * ========================================================= */

    Temperature = (float)raw_temp / 16.0f;


    /* =========================================================
     * 8. SAFETY LIMIT
     * ========================================================= */

    if (Temperature > 40.0f)
    {
        temp_out_of_range = 1;
    }
    else
    {
        temp_out_of_range = 0;
    }


    /*
     * Only NOW declare sensor valid.
     */
    temp_sens_pres = 1;

    return Temperature;
}
