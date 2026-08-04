#ifndef AS7341_C_WRAPPER_H
#define AS7341_C_WRAPPER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------- Channel index mapping for C -------- */
#define AS7341_CH_F1   0
#define AS7341_CH_F2   1
#define AS7341_CH_F3   2
#define AS7341_CH_F4   3
#define AS7341_CH_CLEAR_0 4
#define AS7341_CH_NIR_0   5
#define AS7341_CH_F5   6   // ✅ 555 nm
#define AS7341_CH_F6   7
#define AS7341_CH_F7   8
#define AS7341_CH_F8   9
#define AS7341_CH_CLEAR 10
#define AS7341_CH_NIR   11


/* -------- Gain values for C (DO NOT clash with C++ enums) -------- */
#define AS7341_GAIN_C_0_5X   0
#define AS7341_GAIN_C_1X     1
#define AS7341_GAIN_C_2X     2
#define AS7341_GAIN_C_4X     3
#define AS7341_GAIN_C_8X     4
#define AS7341_GAIN_C_16X    5
#define AS7341_GAIN_C_32X    6
#define AS7341_GAIN_C_64X    7
#define AS7341_GAIN_C_128X   8
#define AS7341_GAIN_C_256X   9
#define AS7341_GAIN_C_512X   10


bool AS7341_LED_Enable(void);
bool AS7341_LED_Disable(void);
bool AS7341_LED_SetCurrent(uint16_t current_mA);



bool AS7341_Init(I2C_HandleTypeDef *hi2c);
bool AS7341_ReadAll(uint16_t *buffer);
uint16_t AS7341_ReadF5(void);

bool AS7341_SetATIME(uint8_t atime);
bool AS7341_SetASTEP(uint16_t astep);
bool AS7341_SetGain(uint8_t gain);

bool AS7341_EnableSpectralMeasurement(bool enable);

//bool AS7341_EnableLED(bool enable);

//bool AS7341_SetLEDCurrent(uint16_t ma);




#ifdef __cplusplus
}
#endif

#endif
