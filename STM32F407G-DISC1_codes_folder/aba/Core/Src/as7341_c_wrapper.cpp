#include "as7341_c_wrapper.h"
#include "Adafruit_AS7341.h"

/* AS7341 I2C address (7-bit << 1) */
#define AS7341_ADDR        (0x39 << 1)

/* Registers (BANK 0) */
#define AS7341_CONFIG_REG  0x70
#define AS7341_LED_REG     0x74

extern I2C_HandleTypeDef hi2c1;

/* Single global driver object */
static Adafruit_AS7341 as7341;

/* ================= BASIC WRAPPERS ================= */

bool AS7341_Init(I2C_HandleTypeDef *hi2c)
{
    return as7341.begin(AS7341_I2CADDR_DEFAULT, hi2c);
}

bool AS7341_ReadAll(uint16_t *buffer)
{
    return as7341.readAllChannels(buffer);
}

uint16_t AS7341_ReadF5(void)
{
    return as7341.getChannel(AS7341_CHANNEL_555nm_F5);
}

bool AS7341_SetATIME(uint8_t atime)
{
    return as7341.setATIME(atime);
}

bool AS7341_SetASTEP(uint16_t astep)
{
    return as7341.setASTEP(astep);
}

bool AS7341_SetGain(uint8_t gain)
{
    return as7341.setGain((as7341_gain_t)gain);
}





bool AS7341_EnableSpectralMeasurement(bool enable)
{
    return as7341.enableSpectralMeasurement(enable);
}

/* ================= LED CONTROL ================= */

/*
 * Enable AS7341 internal LED
 * - CONFIG[3] = LED_SEL (register control)
 * - LED[0]    = LED_EN
 */


bool AS7341_LED_Enable(void)
{
    return as7341.enableLED(true);
}

bool AS7341_LED_Disable(void)
{
    return as7341.enableLED(false);
}

bool AS7341_LED_SetCurrent(uint16_t current_mA)
{
    return as7341.setLEDCurrent(current_mA);
}























//#include "as7341_c_wrapper.h"
//#include "Adafruit_AS7341.h"
//
///* AS7341 I2C address (shifted) */
//#define AS7341_ADDR        (0x39 << 1)
//
///* Registers */
//#define AS7341_CFG0       0xA9
//#define AS7341_CONFIG_REG  0x70
//#define AS7341_LED_REG     0x74
//
//extern I2C_HandleTypeDef hi2c1;
//
//static Adafruit_AS7341 as7341;
//
//
///* ---------- Bank control ---------- */
//static void AS7341_SelectBank1(void)
//{
//    uint8_t v = 0x10;   // CFG0 bit4 = 1 → Bank-1
//    HAL_I2C_Mem_Write(&hi2c1, AS7341_ADDR, AS7341_CFG0,
//                      I2C_MEMADD_SIZE_8BIT, &v, 1, 100);
//}
//
//static void AS7341_SelectBank0(void)
//{
//    uint8_t v = 0x00;   // CFG0 bit4 = 0 → Bank-0
//    HAL_I2C_Mem_Write(&hi2c1, AS7341_ADDR, AS7341_CFG0,
//                      I2C_MEMADD_SIZE_8BIT, &v, 1, 100);
//}
//
//
//bool AS7341_Init(I2C_HandleTypeDef *hi2c)
//{
//    return as7341.begin(AS7341_I2CADDR_DEFAULT, hi2c);
//}
//
//bool AS7341_ReadAll(uint16_t *buffer)
//{
//    return as7341.readAllChannels(buffer);
//}
//
//uint16_t AS7341_ReadF5(void)
//{
//    return as7341.getChannel(AS7341_CHANNEL_555nm_F5);
//}
//
///* -------- NEW CONFIG WRAPPERS -------- */
//
//bool AS7341_SetATIME(uint8_t atime)
//{
//    return as7341.setATIME(atime);
//}
//
//bool AS7341_SetASTEP(uint16_t astep)
//{
//    return as7341.setASTEP(astep);
//}
//
//bool AS7341_SetGain(uint8_t gain)
//{
//    return as7341.setGain((as7341_gain_t)gain);
//}
//
////bool AS7341_EnableLED(bool enable)
////{
////    return as7341.enableLED(enable);
////}
////
////bool AS7341_SetLEDCurrent(uint16_t ma)
////{
////    return as7341.setLEDCurrent(ma);
////}
//
///* ---------- LED ON ---------- */
//void AS7341_LED_Enable(void)
//{
//    uint8_t reg;
//
//    AS7341_SelectBank1();
//
//    /* Enable register-controlled LED (CONFIG bit3) */
//    HAL_I2C_Mem_Read(&hi2c1, AS7341_ADDR, AS7341_CONFIG,
//                     I2C_MEMADD_SIZE_8BIT, &reg, 1, 100);
//    reg |= (1 << 3);   // LED_SEL = 1
//    HAL_I2C_Mem_Write(&hi2c1, AS7341_ADDR, AS7341_CONFIG,
//                      I2C_MEMADD_SIZE_8BIT, &reg, 1, 100);
//
//    /* Turn LED ON (LED bit0) */
//    HAL_I2C_Mem_Read(&hi2c1, AS7341_ADDR, AS7341_LED,
//                     I2C_MEMADD_SIZE_8BIT, &reg, 1, 100);
//    reg |= 0x01;       // LED_ON = 1
//    HAL_I2C_Mem_Write(&hi2c1, AS7341_ADDR, AS7341_LED,
//                      I2C_MEMADD_SIZE_8BIT, &reg, 1, 100);
//
//    AS7341_SelectBank0();
//}
//
///* ---------- LED brightness ---------- */
//void AS7341_LED_SetCurrent(uint8_t current_mA)
//{
//    uint8_t reg;
//    uint8_t code;
//
//    if (current_mA > 258)
//        current_mA = 258;
//
//    code = current_mA / 2;   // 2 mA per step
//
//    AS7341_SelectBank1();
//
//    HAL_I2C_Mem_Read(&hi2c1, AS7341_ADDR, AS7341_LED,
//                     I2C_MEMADD_SIZE_8BIT, &reg, 1, 100);
//
//    reg &= 0x01;              // keep LED_ON
//    reg |= (code << 1);       // set current
//
//    HAL_I2C_Mem_Write(&hi2c1, AS7341_ADDR, AS7341_LED,
//                      I2C_MEMADD_SIZE_8BIT, &reg, 1, 100);
//
//    AS7341_SelectBank0();
//}
//
//
//
//
////#include "as7341_c_wrapper.h"
////#include "Adafruit_AS7341.h"
////
////static Adafruit_AS7341 as7341;
////
////bool AS7341_Init(I2C_HandleTypeDef *hi2c)
////{
////    return as7341.begin(AS7341_I2CADDR_DEFAULT, hi2c);
////}
////
////bool AS7341_ReadAll(uint16_t *buffer)
////{
////    return as7341.readAllChannels(buffer);
////}
////
////uint16_t AS7341_ReadF5(void)
////{
////    return as7341.getChannel(AS7341_CHANNEL_555nm_F5);
////}
