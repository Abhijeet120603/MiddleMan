/*
 * LTR390.c
 *
 *  Created on: Jul 30, 2026
 *      Author: HP
 */


#include "ltr390.h"
#include <stddef.h>

void LTR390_Init(LTR390_HandleTypeDef *ltr, I2C_HandleTypeDef *hi2c) {
    ltr->hi2c = hi2c;
    ltr->i2cAddress = LTR390_ADDRESS;

    // Initialize gain factors
    ltr->gain_factor[0] = 1.0f;
    ltr->gain_factor[1] = 3.0f;
    ltr->gain_factor[2] = 6.0f;
    ltr->gain_factor[3] = 9.0f;
    ltr->gain_factor[4] = 18.0f;

    // Initialize resolution factors
    ltr->res_factor[0] = 4.0f;
    ltr->res_factor[1] = 2.0f;
    ltr->res_factor[2] = 1.0f;
    ltr->res_factor[3] = 0.5f;
    ltr->res_factor[4] = 0.25f;
    ltr->res_factor[5] = 0.03125f;
}

bool LTR390_Begin(LTR390_HandleTypeDef *ltr) {
    uint8_t part_id = LTR390_ReadRegister(ltr, LTR390_PART_ID);

    if ((part_id >> 4) != 0x0B) {
        return false;
    }

    if (!LTR390_Reset(ltr)) {
        return false;
    }

    LTR390_Enable(ltr, true);
    if (!LTR390_IsEnabled(ltr)) {
        return false;
    }

    LTR390_SetGain(ltr, LTR390_GAIN_3);
    LTR390_SetResolution(ltr, LTR390_RESOLUTION_18BIT);

    return true;
}

bool LTR390_Reset(LTR390_HandleTypeDef *ltr) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_MAIN_CTRL);
    reg_val |= 0x10;  // Set reset bit
    LTR390_WriteRegister(ltr, LTR390_MAIN_CTRL, reg_val);

    HAL_Delay(10);

    reg_val = LTR390_ReadRegister(ltr, LTR390_MAIN_CTRL);
    if (reg_val != 0) {
        return false;
    }

    return true;
}

bool LTR390_NewDataAvailable(LTR390_HandleTypeDef *ltr) {
    uint8_t status = LTR390_ReadRegister(ltr, LTR390_MAIN_STATUS);
    status >>= 3;
    status &= 1;
    return (status == 1);
}

uint32_t LTR390_ReadALS(LTR390_HandleTypeDef *ltr) {
    uint8_t _lsb = LTR390_ReadRegister(ltr, LTR390_ALSDATA_LSB);
    uint8_t _msb = LTR390_ReadRegister(ltr, LTR390_ALSDATA_MSB);
    uint8_t _hsb = LTR390_ReadRegister(ltr, LTR390_ALSDATA_HSB);
    _hsb &= 0x0F;
    uint32_t _out = ((uint32_t)_hsb << 16) | ((uint16_t)_msb << 8) | _lsb;
    return _out;
}

uint32_t LTR390_ReadUVS(LTR390_HandleTypeDef *ltr) {
    uint8_t _lsb = LTR390_ReadRegister(ltr, LTR390_UVSDATA_LSB);
    uint8_t _msb = LTR390_ReadRegister(ltr, LTR390_UVSDATA_MSB);
    uint8_t _hsb = LTR390_ReadRegister(ltr, LTR390_UVSDATA_HSB);
    _hsb &= 0x0F;
    uint32_t _out = ((uint32_t)_hsb << 16) | ((uint16_t)_msb << 8) | _lsb;
    return _out;
}

float LTR390_GetLux(LTR390_HandleTypeDef *ltr) {
    uint32_t raw = LTR390_ReadALS(ltr);
    uint8_t _gain = (uint8_t)(LTR390_GetGain(ltr));
    uint8_t _resolution = (uint8_t)(LTR390_GetResolution(ltr));
    float lux = 0.6f * (float)(raw) / (ltr->gain_factor[_gain] * ltr->res_factor[_resolution]) * (float)(WFAC);
    return lux;
}

float LTR390_GetUVI(LTR390_HandleTypeDef *ltr) {
    uint32_t raw = LTR390_ReadUVS(ltr);
    uint8_t _gain = (uint8_t)(LTR390_GetGain(ltr));
    uint8_t _resolution = (uint8_t)(LTR390_GetResolution(ltr));
    float uvi = (float)(raw) / ((ltr->gain_factor[_gain] / ltr->gain_factor[LTR390_GAIN_18]) *
                              (ltr->res_factor[_resolution] / ltr->res_factor[LTR390_RESOLUTION_20BIT]) *
                              (float)(UV_SENSITIVITY)) * (float)(WFAC);
    return uvi;
}

void LTR390_Enable(LTR390_HandleTypeDef *ltr, bool en) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_MAIN_CTRL);
    if (en) {
        reg_val |= (1 << 1);
    } else {
        reg_val &= ~(1 << 1);
    }
    LTR390_WriteRegister(ltr, LTR390_MAIN_CTRL, reg_val);
}

bool LTR390_IsEnabled(LTR390_HandleTypeDef *ltr) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_MAIN_CTRL);
    reg_val >>= 1;
    reg_val &= 1;
    return (reg_val == 1);
}

void LTR390_SetMode(LTR390_HandleTypeDef *ltr, ltr390_mode_t mode) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_MAIN_CTRL);
    reg_val &= 0xF7;  // Clear bit 3
    reg_val |= ((uint8_t)mode << 3);
    LTR390_WriteRegister(ltr, LTR390_MAIN_CTRL, reg_val);
}

ltr390_mode_t LTR390_GetMode(LTR390_HandleTypeDef *ltr) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_MAIN_CTRL);
    reg_val >>= 3;
    reg_val &= 1;
    return (ltr390_mode_t)reg_val;
}

void LTR390_SetGain(LTR390_HandleTypeDef *ltr, ltr390_gain_t gain) {
    LTR390_WriteRegister(ltr, LTR390_GAIN, (uint8_t)gain);
}

ltr390_gain_t LTR390_GetGain(LTR390_HandleTypeDef *ltr) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_GAIN);
    reg_val &= 7;
    return (ltr390_gain_t)reg_val;
}

void LTR390_SetResolution(LTR390_HandleTypeDef *ltr, ltr390_resolution_t res) {
    uint8_t reg_val = 0;
    reg_val |= (res << 4);
    LTR390_WriteRegister(ltr, LTR390_MEAS_RATE, reg_val);
}

ltr390_resolution_t LTR390_GetResolution(LTR390_HandleTypeDef *ltr) {
    uint8_t reg_val = LTR390_ReadRegister(ltr, LTR390_MEAS_RATE);
    reg_val &= 0x70;
    reg_val = 7 & (reg_val >> 4);
    return (ltr390_resolution_t)reg_val;
}

void LTR390_SetThresholds(LTR390_HandleTypeDef *ltr, uint32_t lower, uint32_t higher) {
    uint8_t reg_val;

    reg_val = higher & 0xFF;
    LTR390_WriteRegister(ltr, LTR390_THRESH_UP, reg_val);

    reg_val = (higher >> 8) & 0xFF;
    LTR390_WriteRegister(ltr, LTR390_THRESH_UP + 1, reg_val);

    reg_val = (higher >> 16) & 0x0F;
    LTR390_WriteRegister(ltr, LTR390_THRESH_UP + 2, reg_val);

    reg_val = lower & 0xFF;
    LTR390_WriteRegister(ltr, LTR390_THRESH_LOW, reg_val);

    reg_val = (lower >> 8) & 0xFF;
    LTR390_WriteRegister(ltr, LTR390_THRESH_LOW + 1, reg_val);

    reg_val = (lower >> 16) & 0x0F;
    LTR390_WriteRegister(ltr, LTR390_THRESH_LOW + 2, reg_val);
}

void LTR390_ConfigInterrupt(LTR390_HandleTypeDef *ltr, bool enable,
                           ltr390_mode_t source, uint8_t persistance) {
    uint8_t reg_val = 0;
    reg_val |= (enable << 2) | (1 << 4) | (source << 5);
    LTR390_WriteRegister(ltr, LTR390_INT_CFG, reg_val);

    if (persistance > 0x0F) persistance = 0x0F;
    uint8_t p_val = 0;
    p_val |= persistance << 4;
    LTR390_WriteRegister(ltr, LTR390_INT_PST, p_val);
}

uint8_t LTR390_WriteRegister(LTR390_HandleTypeDef *ltr,
                             uint8_t reg,
                             uint8_t val)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(ltr->hi2c,
                               ltr->i2cAddress << 1,
                               reg,
                               I2C_MEMADD_SIZE_8BIT,
                               &val,
                               1,
                               HAL_MAX_DELAY);

    return (status == HAL_OK);
}

uint8_t LTR390_ReadRegister(LTR390_HandleTypeDef *ltr, uint8_t reg)
{
    uint8_t value = 0;

    HAL_I2C_Mem_Read(ltr->hi2c,
                     ltr->i2cAddress << 1,
                     reg,
                     I2C_MEMADD_SIZE_8BIT,
                     &value,
                     1,
                     HAL_MAX_DELAY);

    return value;
}


