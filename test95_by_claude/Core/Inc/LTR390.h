/*
 * LTR390.h
 *
 *  Created on: Jul 30, 2026
 *      Author: HP
 */

#ifndef INC_LTR390_H_
#define INC_LTR390_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define UV_SENSITIVITY  1400
#define WFAC            1

#define LTR390_ADDRESS 0x53
#define LTR390_MAIN_CTRL 0x00
#define LTR390_MEAS_RATE 0x04
#define LTR390_GAIN 0x05
#define LTR390_PART_ID 0x06
#define LTR390_MAIN_STATUS 0x07
#define LTR390_ALSDATA_LSB 0x0D
#define LTR390_ALSDATA_MSB 0x0E
#define LTR390_ALSDATA_HSB 0x0F
#define LTR390_UVSDATA_LSB 0x10
#define LTR390_UVSDATA_MSB 0x11
#define LTR390_UVSDATA_HSB 0x12
#define LTR390_INT_CFG 0x19
#define LTR390_INT_PST 0x1A
#define LTR390_THRESH_UP 0x21
#define LTR390_THRESH_LOW 0x24

typedef enum {
    LTR390_MODE_ALS,
    LTR390_MODE_UVS,
} ltr390_mode_t;

typedef enum {
    LTR390_GAIN_1 = 0,
    LTR390_GAIN_3,
    LTR390_GAIN_6,
    LTR390_GAIN_9,
    LTR390_GAIN_18,
} ltr390_gain_t;

typedef enum {
    LTR390_RESOLUTION_20BIT,
    LTR390_RESOLUTION_19BIT,
    LTR390_RESOLUTION_18BIT,
    LTR390_RESOLUTION_17BIT,
    LTR390_RESOLUTION_16BIT,
    LTR390_RESOLUTION_13BIT,
} ltr390_resolution_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2cAddress;
    float gain_factor[5];
    float res_factor[6];
} LTR390_HandleTypeDef;

// Initialization
void LTR390_Init(LTR390_HandleTypeDef *ltr, I2C_HandleTypeDef *hi2c);
bool LTR390_Begin(LTR390_HandleTypeDef *ltr);

// Basic operations
bool LTR390_Reset(LTR390_HandleTypeDef *ltr);
void LTR390_Enable(LTR390_HandleTypeDef *ltr, bool en);
bool LTR390_IsEnabled(LTR390_HandleTypeDef *ltr);

// Configuration
void LTR390_SetMode(LTR390_HandleTypeDef *ltr, ltr390_mode_t mode);
ltr390_mode_t LTR390_GetMode(LTR390_HandleTypeDef *ltr);

void LTR390_SetGain(LTR390_HandleTypeDef *ltr, ltr390_gain_t gain);
ltr390_gain_t LTR390_GetGain(LTR390_HandleTypeDef *ltr);

void LTR390_SetResolution(LTR390_HandleTypeDef *ltr, ltr390_resolution_t res);
ltr390_resolution_t LTR390_GetResolution(LTR390_HandleTypeDef *ltr);

// Data reading
bool LTR390_NewDataAvailable(LTR390_HandleTypeDef *ltr);
uint32_t LTR390_ReadALS(LTR390_HandleTypeDef *ltr);
uint32_t LTR390_ReadUVS(LTR390_HandleTypeDef *ltr);

// Calculated values
float LTR390_GetLux(LTR390_HandleTypeDef *ltr);
float LTR390_GetUVI(LTR390_HandleTypeDef *ltr);

// Interrupt configuration
void LTR390_SetThresholds(LTR390_HandleTypeDef *ltr, uint32_t lower, uint32_t higher);
void LTR390_ConfigInterrupt(LTR390_HandleTypeDef *ltr, bool enable,
                           ltr390_mode_t source, uint8_t persistance);

// I2C communication
uint8_t LTR390_WriteRegister(LTR390_HandleTypeDef *ltr, uint8_t reg, uint8_t val);
uint8_t LTR390_ReadRegister(LTR390_HandleTypeDef *ltr, uint8_t reg);


#endif /* INC_LTR390_H_ */
