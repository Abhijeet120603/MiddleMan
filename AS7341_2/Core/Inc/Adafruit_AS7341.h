///*
// * Adafruit_AS7341.h
// *
// *  Created on: Jul 29, 2026
// *      Author: HP
// */
//
//#ifndef INC_ADAFRUIT_AS7341_H_
//#define INC_ADAFRUIT_AS7341_H_
//
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//#include "stm32f401xc.h"
//#include <stdbool.h>
//#include <stdint.h>
//
///* ============================================================ */
///*                     Register Definitions                      */
///* ============================================================ */
//
//#define AS7341_I2CADDR_DEFAULT 0x39 ///< AS7341 default i2c address
//#define AS7341_CHIP_ID 0x09         ///< AS7341 default device id from WHOAMI
//
//#define AS7341_WHOAMI 0x92          ///< Chip ID register
//#define AS7341_CONFIG 0x70          ///< Enables LED control and sets light sensing mode
//#define AS7341_LED 0x74             ///< LED Register; Enables and sets current limit
//#define AS7341_ENABLE 0x80          ///< Main enable register
//#define AS7341_ATIME 0x81           ///< Sets ADC integration step count
//#define AS7341_SP_LOW_TH_L 0x84     ///< Spectral measurement Low Threshold low byte
//#define AS7341_SP_LOW_TH_H 0x85     ///< Spectral measurement Low Threshold high byte
//#define AS7341_SP_HIGH_TH_L 0x86    ///< Spectral measurement High Threshold low byte
//#define AS7341_SP_HIGH_TH_H 0x87    ///< Spectral measurement High Threshold low byte
//#define AS7341_STATUS 0x93          ///< Interrupt status registers
//#define AS7341_CH0_DATA_L 0x95      ///< ADC Channel Data
//#define AS7341_CH0_DATA_H 0x96      ///< ADC Channel Data
//#define AS7341_CH1_DATA_L 0x97      ///< ADC Channel Data
//#define AS7341_CH1_DATA_H 0x98      ///< ADC Channel Data
//#define AS7341_CH2_DATA_L 0x99      ///< ADC Channel Data
//#define AS7341_CH2_DATA_H 0x9A      ///< ADC Channel Data
//#define AS7341_CH3_DATA_L 0x9B      ///< ADC Channel Data
//#define AS7341_CH3_DATA_H 0x9C      ///< ADC Channel Data
//#define AS7341_CH4_DATA_L 0x9D      ///< ADC Channel Data
//#define AS7341_CH4_DATA_H 0x9E      ///< ADC Channel Data
//#define AS7341_CH5_DATA_L 0x9F      ///< ADC Channel Data
//#define AS7341_CH5_DATA_H 0xA0      ///< ADC Channel Data
//#define AS7341_STATUS2 0xA3         ///< Measurement status flags
//#define AS7341_STATUS3 0xA4         ///< Spectral interrupt source
//#define AS7341_CFG0 0xA9            ///< Sets Low power mode, Register bank
//#define AS7341_CFG1 0xAA            ///< Controls ADC Gain
//#define AS7341_CFG6 0xAF            ///< Used to configure SMUX
//#define AS7341_CFG12 0xB5           ///< Spectral threshold channel
//#define AS7341_PERS 0xBD            ///< Interrupt persistence
//#define AS7341_GPIO2 0xBE           ///< GPIO Settings and status
//#define AS7341_ASTEP_L 0xCA         ///< Integration step size low byte
//#define AS7341_ASTEP_H 0xCB         ///< Integration step size high byte
//#define AS7341_FD_STATUS 0xDB       ///< Flicker detection status
//#define AS7341_INTENAB 0xF9         ///< Enables individual interrupt types
//
//#define AS7341_SPECTRAL_INT_HIGH_MSK 0b00100000  ///< High threshold interrupt mask
//#define AS7341_SPECTRAL_INT_LOW_MSK  0b00010000  ///< Low threshold interrupt mask
//
///* ============================================================ */
///*                         Enumerations                         */
///* ============================================================ */
//
///**
// * @brief Allowable gain multipliers for `setGain`
// */
//typedef enum {
//    AS7341_GAIN_0_5X = 0,
//    AS7341_GAIN_1X = 1,
//    AS7341_GAIN_2X = 2,
//    AS7341_GAIN_4X = 3,
//    AS7341_GAIN_8X = 4,
//    AS7341_GAIN_16X = 5,
//    AS7341_GAIN_32X = 6,
//    AS7341_GAIN_64X = 7,
//    AS7341_GAIN_128X = 8,
//    AS7341_GAIN_256X = 9,
//    AS7341_GAIN_512X = 10,
//} as7341_gain_t;
//
///**
// * @brief Available SMUX configuration commands
// */
//typedef enum {
//    AS7341_SMUX_CMD_ROM_RESET = 0,  ///< ROM code initialization of SMUX
//    AS7341_SMUX_CMD_READ = 1,       ///< Read SMUX configuration to RAM from SMUX chain
//    AS7341_SMUX_CMD_WRITE = 2,      ///< Write SMUX configuration from RAM to SMUX chain
//} as7341_smux_cmd_t;
//
///**
// * @brief ADC Channel specifiers for configuration
// */
//typedef enum {
//    AS7341_ADC_CHANNEL_0 = 0,
//    AS7341_ADC_CHANNEL_1,
//    AS7341_ADC_CHANNEL_2,
//    AS7341_ADC_CHANNEL_3,
//    AS7341_ADC_CHANNEL_4,
//    AS7341_ADC_CHANNEL_5,
//} as7341_adc_channel_t;
//
///**
// * @brief Spectral Channel specifiers for configuration and reading
// */
//typedef enum {
//    AS7341_CHANNEL_415nm_F1 = 0,
//    AS7341_CHANNEL_445nm_F2,
//    AS7341_CHANNEL_480nm_F3,
//    AS7341_CHANNEL_515nm_F4,
//    AS7341_CHANNEL_CLEAR_0,
//    AS7341_CHANNEL_NIR_0,
//    AS7341_CHANNEL_555nm_F5,
//    AS7341_CHANNEL_590nm_F6,
//    AS7341_CHANNEL_630nm_F7,
//    AS7341_CHANNEL_680nm_F8,
//    AS7341_CHANNEL_CLEAR,
//    AS7341_CHANNEL_NIR,
//} as7341_color_channel_t;
//
///**
// * @brief Interrupt persistence cycle counts
// */
//typedef enum {
//    AS7341_INT_COUNT_ALL = 0,
//    AS7341_INT_COUNT_1 = 1,
//    AS7341_INT_COUNT_2 = 2,
//    AS7341_INT_COUNT_3 = 3,
//    AS7341_INT_COUNT_5 = 4,
//    AS7341_INT_COUNT_10 = 5,
//    AS7341_INT_COUNT_15 = 6,
//    AS7341_INT_COUNT_20 = 7,
//    AS7341_INT_COUNT_25 = 8,
//    AS7341_INT_COUNT_30 = 9,
//    AS7341_INT_COUNT_35 = 10,
//    AS7341_INT_COUNT_40 = 11,
//    AS7341_INT_COUNT_45 = 12,
//    AS7341_INT_COUNT_50 = 13,
//    AS7341_INT_COUNT_55 = 14,
//    AS7341_INT_COUNT_60 = 15,
//} as7341_int_cycle_count_t;
//
///**
// * @brief GPIO pin directions
// */
//typedef enum {
//    AS7341_GPIO_OUTPUT = 0, ///< GPIO as open drain output
//    AS7341_GPIO_INPUT = 1,  ///< GPIO as high-impedance input
//} as7341_gpio_dir_t;
//
///**
// * @brief Wait states for async reading
// */
//typedef enum {
//    AS7341_WAITING_START = 0,
//    AS7341_WAITING_LOW,
//    AS7341_WAITING_HIGH,
//    AS7341_WAITING_DONE,
//} as7341_waiting_t;
//
///* ============================================================ */
///*                         Device Structure                     */
///* ============================================================ */
//
///**
// * @brief AS7341 device instance structure
// */
//typedef struct {
//    I2C_HandleTypeDef *i2c_han;          ///< Pointer to I2C bus interface
//    uint8_t i2c_addr;                    ///< I2C device address (shifted)
//    uint8_t last_spectral_int_source;    ///< Last spectral interrupt source value
//    uint16_t _channel_readings[12];      ///< Internal channel readings buffer
//    as7341_waiting_t _readingState;      ///< Async reading state
//} Adafruit_AS7341_t;
//
///* ============================================================ */
///*                    Public API Functions                      */
///* ============================================================ */
//
///* ---- Initialization ---- */
//void Adafruit_AS7341_Init(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_begin(Adafruit_AS7341_t *dev, uint8_t i2c_address,
//                           I2C_HandleTypeDef *i2c_handle, int32_t sensor_id);
//
///* ---- Power and Enable ---- */
//void Adafruit_AS7341_powerEnable(Adafruit_AS7341_t *dev, bool enable_power);
//void Adafruit_AS7341_disableAll(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_enableSpectralMeasurement(Adafruit_AS7341_t *dev, bool enable_measurement);
//bool Adafruit_AS7341_enableLED(Adafruit_AS7341_t *dev, bool enable_led);
//bool Adafruit_AS7341_setLEDCurrent(Adafruit_AS7341_t *dev, uint16_t led_current_ma);
//
///* ---- Reading Data ---- */
//uint16_t Adafruit_AS7341_readChannel(Adafruit_AS7341_t *dev, as7341_adc_channel_t channel);
//uint16_t Adafruit_AS7341_getChannel(Adafruit_AS7341_t *dev, as7341_color_channel_t channel);
//bool Adafruit_AS7341_readAllChannels(Adafruit_AS7341_t *dev, uint16_t *readings_buffer);
//bool Adafruit_AS7341_readAllChannelsToBuffer(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_getAllChannels(Adafruit_AS7341_t *dev, uint32_t *readings_buffer);
//bool Adafruit_AS7341_getIsDataReady(Adafruit_AS7341_t *dev);
//void Adafruit_AS7341_delayForData(Adafruit_AS7341_t *dev, uint32_t waitTime);
//
///* ---- Async Reading ---- */
//bool Adafruit_AS7341_startReading(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_checkReadingProgress(Adafruit_AS7341_t *dev);
//
///* ---- Configuration ---- */
//bool Adafruit_AS7341_setATIME(Adafruit_AS7341_t *dev, uint8_t atime_value);
//uint8_t Adafruit_AS7341_getATIME(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_setASTEP(Adafruit_AS7341_t *dev, uint16_t astep_value);
//uint16_t Adafruit_AS7341_getASTEP(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_setGain(Adafruit_AS7341_t *dev, as7341_gain_t gain_value);
//as7341_gain_t Adafruit_AS7341_getGain(Adafruit_AS7341_t *dev);
//long Adafruit_AS7341_getTINT(Adafruit_AS7341_t *dev);
//float Adafruit_AS7341_toBasicCounts(Adafruit_AS7341_t *dev, uint16_t raw);
//
///* ---- Bank and SMUX ---- */
//bool Adafruit_AS7341_setBank(Adafruit_AS7341_t *dev, bool low);
//void Adafruit_AS7341_setup_F1F4_Clear_NIR(Adafruit_AS7341_t *dev);
//void Adafruit_AS7341_setup_F5F8_Clear_NIR(Adafruit_AS7341_t *dev);
//
///* ---- Interrupts ---- */
//bool Adafruit_AS7341_setLowThreshold(Adafruit_AS7341_t *dev, uint16_t low_threshold);
//uint16_t Adafruit_AS7341_getLowThreshold(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_setHighThreshold(Adafruit_AS7341_t *dev, uint16_t high_threshold);
//uint16_t Adafruit_AS7341_getHighThreshold(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_enableSpectralInterrupt(Adafruit_AS7341_t *dev, bool enable_int);
//bool Adafruit_AS7341_enableSystemInterrupt(Adafruit_AS7341_t *dev, bool enable_int);
//bool Adafruit_AS7341_setAPERS(Adafruit_AS7341_t *dev, as7341_int_cycle_count_t cycle_count);
//bool Adafruit_AS7341_setSpectralThresholdChannel(Adafruit_AS7341_t *dev, as7341_adc_channel_t channel);
//uint8_t Adafruit_AS7341_getInterruptStatus(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_clearInterruptStatus(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_spectralInterruptTriggered(Adafruit_AS7341_t *dev);
//uint8_t Adafruit_AS7341_spectralInterruptSource(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_spectralLowTriggered(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_spectralHighTriggered(Adafruit_AS7341_t *dev);
//
///* ---- GPIO ---- */
//as7341_gpio_dir_t Adafruit_AS7341_getGPIODirection(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_setGPIODirection(Adafruit_AS7341_t *dev, as7341_gpio_dir_t gpio_direction);
//bool Adafruit_AS7341_getGPIOInverted(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_setGPIOInverted(Adafruit_AS7341_t *dev, bool gpio_inverted);
//bool Adafruit_AS7341_getGPIOValue(Adafruit_AS7341_t *dev);
//bool Adafruit_AS7341_setGPIOValue(Adafruit_AS7341_t *dev, bool gpio_high);
//
///* ---- Flicker Detection ---- */
//uint16_t Adafruit_AS7341_detectFlickerHz(Adafruit_AS7341_t *dev);
//
///* ---- Internal (for SMUX) ---- */
//bool Adafruit_AS7341_enableSMUX(Adafruit_AS7341_t *dev);
//
///* ============================================================ */
///*                  Low-Level I2C Functions                     */
///* ============================================================ */
//
//bool Adafruit_AS7341_writeRegister(Adafruit_AS7341_t *dev, uint8_t mem_addr, uint8_t *val, uint16_t size);
//bool Adafruit_AS7341_writeRegisterByte(Adafruit_AS7341_t *dev, uint8_t mem_addr, uint8_t val);
//bool Adafruit_AS7341_readRegister(Adafruit_AS7341_t *dev, uint16_t mem_addr, uint8_t *dest, uint16_t size);
//uint8_t Adafruit_AS7341_readRegisterByte(Adafruit_AS7341_t *dev, uint16_t mem_addr);
//uint8_t Adafruit_AS7341_modifyBitInByte(uint8_t var, uint8_t value, uint8_t pos);
//uint8_t Adafruit_AS7341_checkRegisterBit(Adafruit_AS7341_t *dev, uint16_t reg, uint8_t pos);
//bool Adafruit_AS7341_modifyRegisterBit(Adafruit_AS7341_t *dev, uint16_t reg, bool value, uint8_t pos);
//bool Adafruit_AS7341_modifyRegisterMultipleBit(Adafruit_AS7341_t *dev, uint16_t reg, uint8_t value,
//                                               uint8_t pos, uint8_t bits);
//
///* ============================================================ */
///*                     Internal Functions                       */
///* ============================================================ */
//
//bool Adafruit_AS7341__init(Adafruit_AS7341_t *dev, int32_t sensor_id);
//void Adafruit_AS7341_setSMUXLowChannels(Adafruit_AS7341_t *dev, bool f1_f4);
//bool Adafruit_AS7341_setSMUXCommand(Adafruit_AS7341_t *dev, as7341_smux_cmd_t command);
//bool Adafruit_AS7341_enableFlickerDetection(Adafruit_AS7341_t *dev, bool enable_fd);
//void Adafruit_AS7341_FDConfig(Adafruit_AS7341_t *dev);
//int8_t Adafruit_AS7341_getFlickerDetectStatus(Adafruit_AS7341_t *dev);
//
//#ifdef __cplusplus
//}
//#endif
//
//
//#endif /* INC_ADAFRUIT_AS7341_H_ */






/*!
 *  @file Adafruit_AS7341.h
 *  @mainpage Adafruit AS7341 11-Channel Spectral Sensor - C Version
 *
 *  I2C Driver for the AS7341 11-Channel Spectral Sensor
 *  Converted from C++ to C for STM32 projects
 */

#ifndef _ADAFRUIT_AS7341_H
#define _ADAFRUIT_AS7341_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"  /* <-- ADD THIS for I2C_HandleTypeDef and HAL functions */

/* ============================================================ */
/*                     Register Definitions                      */
/* ============================================================ */

#define AS7341_I2CADDR_DEFAULT 0x39 ///< AS7341 default i2c address
#define AS7341_CHIP_ID 0x09         ///< AS7341 default device id from WHOAMI

#define AS7341_WHOAMI 0x92          ///< Chip ID register
#define AS7341_CONFIG 0x70          ///< Enables LED control and sets light sensing mode
#define AS7341_LED 0x74             ///< LED Register; Enables and sets current limit
#define AS7341_ENABLE 0x80          ///< Main enable register
#define AS7341_ATIME 0x81           ///< Sets ADC integration step count
#define AS7341_SP_LOW_TH_L 0x84     ///< Spectral measurement Low Threshold low byte
#define AS7341_SP_LOW_TH_H 0x85     ///< Spectral measurement Low Threshold high byte
#define AS7341_SP_HIGH_TH_L 0x86    ///< Spectral measurement High Threshold low byte
#define AS7341_SP_HIGH_TH_H 0x87    ///< Spectral measurement High Threshold low byte
#define AS7341_STATUS 0x93          ///< Interrupt status registers
#define AS7341_CH0_DATA_L 0x95      ///< ADC Channel Data
#define AS7341_CH0_DATA_H 0x96      ///< ADC Channel Data
#define AS7341_CH1_DATA_L 0x97      ///< ADC Channel Data
#define AS7341_CH1_DATA_H 0x98      ///< ADC Channel Data
#define AS7341_CH2_DATA_L 0x99      ///< ADC Channel Data
#define AS7341_CH2_DATA_H 0x9A      ///< ADC Channel Data
#define AS7341_CH3_DATA_L 0x9B      ///< ADC Channel Data
#define AS7341_CH3_DATA_H 0x9C      ///< ADC Channel Data
#define AS7341_CH4_DATA_L 0x9D      ///< ADC Channel Data
#define AS7341_CH4_DATA_H 0x9E      ///< ADC Channel Data
#define AS7341_CH5_DATA_L 0x9F      ///< ADC Channel Data
#define AS7341_CH5_DATA_H 0xA0      ///< ADC Channel Data
#define AS7341_STATUS2 0xA3         ///< Measurement status flags
#define AS7341_STATUS3 0xA4         ///< Spectral interrupt source
#define AS7341_CFG0 0xA9            ///< Sets Low power mode, Register bank
#define AS7341_CFG1 0xAA            ///< Controls ADC Gain
#define AS7341_CFG6 0xAF            ///< Used to configure SMUX
#define AS7341_CFG12 0xB5           ///< Spectral threshold channel
#define AS7341_PERS 0xBD            ///< Interrupt persistence
#define AS7341_GPIO2 0xBE           ///< GPIO Settings and status
#define AS7341_ASTEP_L 0xCA         ///< Integration step size low byte
#define AS7341_ASTEP_H 0xCB         ///< Integration step size high byte
#define AS7341_FD_STATUS 0xDB       ///< Flicker detection status
#define AS7341_INTENAB 0xF9         ///< Enables individual interrupt types

#define AS7341_SPECTRAL_INT_HIGH_MSK 0b00100000  ///< High threshold interrupt mask
#define AS7341_SPECTRAL_INT_LOW_MSK  0b00010000  ///< Low threshold interrupt mask

/* ============================================================ */
/*                         Enumerations                         */
/* ============================================================ */

/**
 * @brief Allowable gain multipliers for `setGain`
 */
typedef enum {
    AS7341_GAIN_0_5X = 0,
    AS7341_GAIN_1X = 1,
    AS7341_GAIN_2X = 2,
    AS7341_GAIN_4X = 3,
    AS7341_GAIN_8X = 4,
    AS7341_GAIN_16X = 5,
    AS7341_GAIN_32X = 6,
    AS7341_GAIN_64X = 7,
    AS7341_GAIN_128X = 8,
    AS7341_GAIN_256X = 9,
    AS7341_GAIN_512X = 10,
} as7341_gain_t;

/**
 * @brief Available SMUX configuration commands
 */
typedef enum {
    AS7341_SMUX_CMD_ROM_RESET = 0,  ///< ROM code initialization of SMUX
    AS7341_SMUX_CMD_READ = 1,       ///< Read SMUX configuration to RAM from SMUX chain
    AS7341_SMUX_CMD_WRITE = 2,      ///< Write SMUX configuration from RAM to SMUX chain
} as7341_smux_cmd_t;

/**
 * @brief ADC Channel specifiers for configuration
 */
typedef enum {
    AS7341_ADC_CHANNEL_0 = 0,
    AS7341_ADC_CHANNEL_1,
    AS7341_ADC_CHANNEL_2,
    AS7341_ADC_CHANNEL_3,
    AS7341_ADC_CHANNEL_4,
    AS7341_ADC_CHANNEL_5,
} as7341_adc_channel_t;

/**
 * @brief Spectral Channel specifiers for configuration and reading
 */
typedef enum {
    AS7341_CHANNEL_415nm_F1 = 0,
    AS7341_CHANNEL_445nm_F2,
    AS7341_CHANNEL_480nm_F3,
    AS7341_CHANNEL_515nm_F4,
    AS7341_CHANNEL_CLEAR_0,
    AS7341_CHANNEL_NIR_0,
    AS7341_CHANNEL_555nm_F5,
    AS7341_CHANNEL_590nm_F6,
    AS7341_CHANNEL_630nm_F7,
    AS7341_CHANNEL_680nm_F8,
    AS7341_CHANNEL_CLEAR,
    AS7341_CHANNEL_NIR,
} as7341_color_channel_t;

/**
 * @brief Interrupt persistence cycle counts
 */
typedef enum {
    AS7341_INT_COUNT_ALL = 0,
    AS7341_INT_COUNT_1 = 1,
    AS7341_INT_COUNT_2 = 2,
    AS7341_INT_COUNT_3 = 3,
    AS7341_INT_COUNT_5 = 4,
    AS7341_INT_COUNT_10 = 5,
    AS7341_INT_COUNT_15 = 6,
    AS7341_INT_COUNT_20 = 7,
    AS7341_INT_COUNT_25 = 8,
    AS7341_INT_COUNT_30 = 9,
    AS7341_INT_COUNT_35 = 10,
    AS7341_INT_COUNT_40 = 11,
    AS7341_INT_COUNT_45 = 12,
    AS7341_INT_COUNT_50 = 13,
    AS7341_INT_COUNT_55 = 14,
    AS7341_INT_COUNT_60 = 15,
} as7341_int_cycle_count_t;

/**
 * @brief GPIO pin directions
 */
typedef enum {
    AS7341_GPIO_OUTPUT = 0, ///< GPIO as open drain output
    AS7341_GPIO_INPUT = 1,  ///< GPIO as high-impedance input
} as7341_gpio_dir_t;

/**
 * @brief Wait states for async reading
 */
typedef enum {
    AS7341_WAITING_START = 0,
    AS7341_WAITING_LOW,
    AS7341_WAITING_HIGH,
    AS7341_WAITING_DONE,
} as7341_waiting_t;

/* ============================================================ */
/*                         Device Structure                     */
/* ============================================================ */

/**
 * @brief AS7341 device instance structure
 */
typedef struct {
    I2C_HandleTypeDef *i2c_han;          ///< Pointer to I2C bus interface
    uint8_t i2c_addr;                    ///< I2C device address (shifted)
    uint8_t last_spectral_int_source;    ///< Last spectral interrupt source value
    uint16_t _channel_readings[12];      ///< Internal channel readings buffer
    as7341_waiting_t _readingState;      ///< Async reading state
} Adafruit_AS7341_t;

/* ============================================================ */
/*                    Public API Functions                      */
/* ============================================================ */

/* ---- Initialization ---- */
void Adafruit_AS7341_Init(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_begin(Adafruit_AS7341_t *dev, uint8_t i2c_address,
                           I2C_HandleTypeDef *i2c_handle, int32_t sensor_id);

/* ---- Power and Enable ---- */
void Adafruit_AS7341_powerEnable(Adafruit_AS7341_t *dev, bool enable_power);
void Adafruit_AS7341_disableAll(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_enableSpectralMeasurement(Adafruit_AS7341_t *dev, bool enable_measurement);
bool Adafruit_AS7341_enableLED(Adafruit_AS7341_t *dev, bool enable_led);
bool Adafruit_AS7341_setLEDCurrent(Adafruit_AS7341_t *dev, uint16_t led_current_ma);

/* ---- Reading Data ---- */
uint16_t Adafruit_AS7341_readChannel(Adafruit_AS7341_t *dev, as7341_adc_channel_t channel);
uint16_t Adafruit_AS7341_getChannel(Adafruit_AS7341_t *dev, as7341_color_channel_t channel);
bool Adafruit_AS7341_readAllChannels(Adafruit_AS7341_t *dev, uint16_t *readings_buffer);
bool Adafruit_AS7341_readAllChannelsToBuffer(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_getAllChannels(Adafruit_AS7341_t *dev, uint32_t *readings_buffer);
bool Adafruit_AS7341_getIsDataReady(Adafruit_AS7341_t *dev);
void Adafruit_AS7341_delayForData(Adafruit_AS7341_t *dev, uint32_t waitTime);

/* ---- Async Reading ---- */
bool Adafruit_AS7341_startReading(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_checkReadingProgress(Adafruit_AS7341_t *dev);

/* ---- Configuration ---- */
bool Adafruit_AS7341_setATIME(Adafruit_AS7341_t *dev, uint8_t atime_value);
uint8_t Adafruit_AS7341_getATIME(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_setASTEP(Adafruit_AS7341_t *dev, uint16_t astep_value);
uint16_t Adafruit_AS7341_getASTEP(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_setGain(Adafruit_AS7341_t *dev, as7341_gain_t gain_value);
as7341_gain_t Adafruit_AS7341_getGain(Adafruit_AS7341_t *dev);
long Adafruit_AS7341_getTINT(Adafruit_AS7341_t *dev);
float Adafruit_AS7341_toBasicCounts(Adafruit_AS7341_t *dev, uint16_t raw);

/* ---- Bank and SMUX ---- */
bool Adafruit_AS7341_setBank(Adafruit_AS7341_t *dev, bool low);
void Adafruit_AS7341_setup_F1F4_Clear_NIR(Adafruit_AS7341_t *dev);
void Adafruit_AS7341_setup_F5F8_Clear_NIR(Adafruit_AS7341_t *dev);

/* ---- Interrupts ---- */
bool Adafruit_AS7341_setLowThreshold(Adafruit_AS7341_t *dev, uint16_t low_threshold);
uint16_t Adafruit_AS7341_getLowThreshold(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_setHighThreshold(Adafruit_AS7341_t *dev, uint16_t high_threshold);
uint16_t Adafruit_AS7341_getHighThreshold(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_enableSpectralInterrupt(Adafruit_AS7341_t *dev, bool enable_int);
bool Adafruit_AS7341_enableSystemInterrupt(Adafruit_AS7341_t *dev, bool enable_int);
bool Adafruit_AS7341_setAPERS(Adafruit_AS7341_t *dev, as7341_int_cycle_count_t cycle_count);
bool Adafruit_AS7341_setSpectralThresholdChannel(Adafruit_AS7341_t *dev, as7341_adc_channel_t channel);
uint8_t Adafruit_AS7341_getInterruptStatus(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_clearInterruptStatus(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_spectralInterruptTriggered(Adafruit_AS7341_t *dev);
uint8_t Adafruit_AS7341_spectralInterruptSource(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_spectralLowTriggered(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_spectralHighTriggered(Adafruit_AS7341_t *dev);

/* ---- GPIO ---- */
as7341_gpio_dir_t Adafruit_AS7341_getGPIODirection(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_setGPIODirection(Adafruit_AS7341_t *dev, as7341_gpio_dir_t gpio_direction);
bool Adafruit_AS7341_getGPIOInverted(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_setGPIOInverted(Adafruit_AS7341_t *dev, bool gpio_inverted);
bool Adafruit_AS7341_getGPIOValue(Adafruit_AS7341_t *dev);
bool Adafruit_AS7341_setGPIOValue(Adafruit_AS7341_t *dev, bool gpio_high);

/* ---- Flicker Detection ---- */
uint16_t Adafruit_AS7341_detectFlickerHz(Adafruit_AS7341_t *dev);

/* ---- Internal (for SMUX) ---- */
bool Adafruit_AS7341_enableSMUX(Adafruit_AS7341_t *dev);

/* ============================================================ */
/*                  Low-Level I2C Functions                     */
/* ============================================================ */

bool Adafruit_AS7341_writeRegister(Adafruit_AS7341_t *dev, uint8_t mem_addr, uint8_t *val, uint16_t size);
bool Adafruit_AS7341_writeRegisterByte(Adafruit_AS7341_t *dev, uint8_t mem_addr, uint8_t val);
bool Adafruit_AS7341_readRegister(Adafruit_AS7341_t *dev, uint16_t mem_addr, uint8_t *dest, uint16_t size);
uint8_t Adafruit_AS7341_readRegisterByte(Adafruit_AS7341_t *dev, uint16_t mem_addr);
uint8_t Adafruit_AS7341_modifyBitInByte(uint8_t var, uint8_t value, uint8_t pos);
uint8_t Adafruit_AS7341_checkRegisterBit(Adafruit_AS7341_t *dev, uint16_t reg, uint8_t pos);
bool Adafruit_AS7341_modifyRegisterBit(Adafruit_AS7341_t *dev, uint16_t reg, bool value, uint8_t pos);
bool Adafruit_AS7341_modifyRegisterMultipleBit(Adafruit_AS7341_t *dev, uint16_t reg, uint8_t value,
                                               uint8_t pos, uint8_t bits);

/* ============================================================ */
/*                     Internal Functions                       */
/* ============================================================ */

bool Adafruit_AS7341__init(Adafruit_AS7341_t *dev, int32_t sensor_id);
void Adafruit_AS7341_setSMUXLowChannels(Adafruit_AS7341_t *dev, bool f1_f4);
bool Adafruit_AS7341_setSMUXCommand(Adafruit_AS7341_t *dev, as7341_smux_cmd_t command);
bool Adafruit_AS7341_enableFlickerDetection(Adafruit_AS7341_t *dev, bool enable_fd);
void Adafruit_AS7341_FDConfig(Adafruit_AS7341_t *dev);
int8_t Adafruit_AS7341_getFlickerDetectStatus(Adafruit_AS7341_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* _ADAFRUIT_AS7341_H */
