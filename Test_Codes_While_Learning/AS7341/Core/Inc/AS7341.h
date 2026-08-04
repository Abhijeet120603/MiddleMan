/*
 * AS7341.h
 *
 *  Created on: Aug 14, 2025
 *      Author: abhij
 */

#ifndef INC_AS7341_H_
#define INC_AS7341_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

// Gain settings
typedef enum {
    GAIN_0_5X = 0,
    GAIN_1X,
    GAIN_2X,
    GAIN_4X,
    GAIN_8X,
    GAIN_16X,
    GAIN_32X,
    GAIN_64X,
    GAIN_128X,
    GAIN_256X,
    GAIN_512X
} as7341_gain_t;

// AS7341 device structure
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t address;
    bool low_channels_configured;
    bool high_channels_configured;
    bool flicker_detection_1k_configured;
    uint8_t buffer[2];
} as7341_t;

// Public functions
bool as7341_init(as7341_t *dev, I2C_HandleTypeDef *hi2c, uint8_t address);
void as7341_initialize(as7341_t *dev);
bool as7341_set_gain(as7341_t *dev, as7341_gain_t gain);
bool as7341_configure_f1_f4(as7341_t *dev);
bool as7341_configure_f5_f8(as7341_t *dev);
bool as7341_read_all_channels(as7341_t *dev, uint16_t *channels);
bool as7341_read_channel_415nm(as7341_t *dev, uint16_t *value);
bool as7341_set_led_current(as7341_t *dev, uint16_t current_mA);
bool as7341_set_led(as7341_t *dev, bool enable);

#endif /* INC_AS7341_H_ */
