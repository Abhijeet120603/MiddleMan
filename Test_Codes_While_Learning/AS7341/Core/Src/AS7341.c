/*
 * AS7341.c
 *
 *  Created on: Aug 14, 2025
 *      Author: abhij
 */


#include "as7341.h"
#include "i2c.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Register addresses
#define AS7341_WHOAMI           0x92
#define AS7341_CONFIG           0x70
#define AS7341_GPIO             0x73
#define AS7341_LED              0x74
#define AS7341_ENABLE           0x80
#define AS7341_ATIME            0x81
#define AS7341_SP_LOW_TH_L      0x84
#define AS7341_SP_LOW_TH_H      0x85
#define AS7341_SP_HIGH_TH_L     0x86
#define AS7341_SP_HIGH_TH_H     0x87
#define AS7341_STATUS           0x93
#define AS7341_ASTATUS          0x94
#define AS7341_CH0_DATA_L       0x95
#define AS7341_CH0_DATA_H       0x96
#define AS7341_CH1_DATA_L       0x97
#define AS7341_CH1_DATA_H       0x98
#define AS7341_CH2_DATA_L       0x99
#define AS7341_CH2_DATA_H       0x9A
#define AS7341_CH3_DATA_L       0x9B
#define AS7341_CH3_DATA_H       0x9C
#define AS7341_CH4_DATA_L       0x9D
#define AS7341_CH4_DATA_H       0x9E
#define AS7341_CH5_DATA_L       0x9F
#define AS7341_CH5_DATA_H       0xA0
#define AS7341_STATUS2          0xA3
#define AS7341_STATUS3          0xA4
#define AS7341_CFG0             0xA9
#define AS7341_CFG1             0xAA
#define AS7341_CFG6             0xAF
#define AS7341_CFG9             0xB2
#define AS7341_CFG12            0xB5
#define AS7341_PERS             0xBD
#define AS7341_GPIO2            0xBE
#define AS7341_ASTEP_L          0xCA
#define AS7341_ASTEP_H          0xCB
#define AS7341_FD_TIME1         0xD8
#define AS7341_FD_TIME2         0xDA
#define AS7341_FD_STATUS        0xDB
#define AS7341_INTENAB          0xF9
#define AS7341_CONTROL          0xFA
#define AS7341_FD_CFG0          0xD7

// Constants
#define AS7341_DEVICE_ID        0b001001
#define AS7341_I2CADDR_DEFAULT  0x39
#define AS7341_CHIP_ID          0x09

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

// SMUX output settings
typedef enum {
    SMUX_OUT_DISABLED = 0,
    SMUX_OUT_ADC0,
    SMUX_OUT_ADC1,
    SMUX_OUT_ADC2,
    SMUX_OUT_ADC3,
    SMUX_OUT_ADC4,
    SMUX_OUT_ADC5
} smux_out_t;

// SMUX input settings
typedef enum {
    SMUX_IN_NC_F3L = 0,
    SMUX_IN_F1L_NC,
    SMUX_IN_NC_NC0,
    SMUX_IN_NC_F8L,
    SMUX_IN_F6L_NC,
    SMUX_IN_F2L_F4L,
    SMUX_IN_NC_F5L,
    SMUX_IN_F7L_NC,
    SMUX_IN_NC_CL,
    SMUX_IN_NC_F5R,
    SMUX_IN_F7R_NC,
    SMUX_IN_NC_NC1,
    SMUX_IN_NC_F2R,
    SMUX_IN_F4R_NC,
    SMUX_IN_F8R_F6R,
    SMUX_IN_NC_F3R,
    SMUX_IN_F1R_EXT_GPIO,
    SMUX_IN_EXT_INT_CR,
    SMUX_IN_NC_DARK,
    SMUX_IN_NIR_F
} smux_in_t;

// AS7341 device structure
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t address;
    bool low_channels_configured;
    bool high_channels_configured;
    bool flicker_detection_1k_configured;
    uint8_t buffer[2];
} as7341_t;

// Private function prototypes
static bool as7341_write_register(as7341_t *dev, uint8_t reg, uint8_t value);
static bool as7341_read_register(as7341_t *dev, uint8_t reg, uint8_t *value);
static bool as7341_read_registers(as7341_t *dev, uint8_t reg, uint8_t *data, uint8_t length);
static bool as7341_set_smux(as7341_t *dev, smux_in_t smux_addr, smux_out_t smux_out1, smux_out_t smux_out2);
static void as7341_f1f4_clear_nir(as7341_t *dev);
static void as7341_f5f8_clear_nir(as7341_t *dev);
static bool as7341_wait_for_data(as7341_t *dev, uint32_t timeout);
static bool as7341_set_bank(as7341_t *dev, bool low_bank);

// Public functions

/**
 * @brief Initialize the AS7341 sensor
 * @param dev Pointer to AS7341 device structure
 * @param hi2c Pointer to I2C handle
 * @param address I2C address of the sensor
 * @return True if initialization succeeded, false otherwise
 */
bool as7341_init(as7341_t *dev, I2C_HandleTypeDef *hi2c, uint8_t address) {
    dev->hi2c = hi2c;
    dev->address = address;
    dev->low_channels_configured = false;
    dev->high_channels_configured = false;
    dev->flicker_detection_1k_configured = false;

    // Check device ID
    uint8_t device_id;
    if (!as7341_read_register(dev, AS7341_WHOAMI, &device_id)) {
        return false;
    }

    if ((device_id >> 2) != AS7341_DEVICE_ID) {
        return false;
    }

    // Configure default settings
    as7341_initialize(dev);

    return true;
}

/**
 * @brief Configure default settings
 * @param dev Pointer to AS7341 device structure
 */
void as7341_initialize(as7341_t *dev) {
    // Enable power
    as7341_write_register(dev, AS7341_ENABLE, 0x01);

    // Enable LED control
    as7341_write_register(dev, AS7341_CONFIG, 0x08);

    // Set integration time
    as7341_write_register(dev, AS7341_ATIME, 100);

    // Set integration step size
    as7341_write_register(dev, AS7341_ASTEP_L, 0xE7);
    as7341_write_register(dev, AS7341_ASTEP_H, 0x03);

    // Set gain to 128X
    as7341_set_gain(dev, GAIN_128X);
}

/**
 * @brief Set the ADC gain
 * @param dev Pointer to AS7341 device structure
 * @param gain Gain value from as7341_gain_t enum
 * @return True if successful, false otherwise
 */
bool as7341_set_gain(as7341_t *dev, as7341_gain_t gain) {
    if (gain > GAIN_512X) {
        return false;
    }
    return as7341_write_register(dev, AS7341_CFG1, gain);
}

/**
 * @brief Configure sensor to read F1-F4 channels
 * @param dev Pointer to AS7341 device structure
 * @return True if successful, false otherwise
 */
bool as7341_configure_f1_f4(as7341_t *dev) {
    if (dev->low_channels_configured) {
        return true;
    }

    dev->high_channels_configured = false;
    dev->flicker_detection_1k_configured = false;

    // Disable spectral measurement
    as7341_write_register(dev, AS7341_ENABLE, 0x01);

    // Set SMUX command
    as7341_write_register(dev, AS7341_CFG6, 0x10);

    // Configure SMUX
    as7341_f1f4_clear_nir(dev);

    // Enable SMUX
    as7341_write_register(dev, AS7341_ENABLE, 0x11);

    // Enable spectral measurement
    as7341_write_register(dev, AS7341_ENABLE, 0x13);

    dev->low_channels_configured = true;
    return as7341_wait_for_data(dev, 100);
}

/**
 * @brief Configure sensor to read F5-F8 channels
 * @param dev Pointer to AS7341 device structure
 * @return True if successful, false otherwise
 */
bool as7341_configure_f5_f8(as7341_t *dev) {
    if (dev->high_channels_configured) {
        return true;
    }

    dev->low_channels_configured = false;
    dev->flicker_detection_1k_configured = false;

    // Disable spectral measurement
    as7341_write_register(dev, AS7341_ENABLE, 0x01);

    // Set SMUX command
    as7341_write_register(dev, AS7341_CFG6, 0x10);

    // Configure SMUX
    as7341_f5f8_clear_nir(dev);

    // Enable SMUX
    as7341_write_register(dev, AS7341_ENABLE, 0x11);

    // Enable spectral measurement
    as7341_write_register(dev, AS7341_ENABLE, 0x13);

    dev->high_channels_configured = true;
    return as7341_wait_for_data(dev, 100);
}

/**
 * @brief Read all channel data
 * @param dev Pointer to AS7341 device structure
 * @param channels Array to store channel data (must be at least 6 elements)
 * @return True if successful, false otherwise
 */
bool as7341_read_all_channels(as7341_t *dev, uint16_t *channels) {
    // First read F1-F4
    if (!as7341_configure_f1_f4(dev)) {
        return false;
    }

    uint8_t data[13];
    if (!as7341_read_registers(dev, AS7341_ASTATUS, data, 13)) {
        return false;
    }

    // Channels 0-3 (F1-F4)
    channels[0] = (data[2] << 8) | data[1];
    channels[1] = (data[4] << 8) | data[3];
    channels[2] = (data[6] << 8) | data[5];
    channels[3] = (data[8] << 8) | data[7];

    // Now read F5-F8
    if (!as7341_configure_f5_f8(dev)) {
        return false;
    }

    if (!as7341_read_registers(dev, AS7341_ASTATUS, data, 13)) {
        return false;
    }

    // Channels 0-3 (F5-F8)
    channels[4] = (data[2] << 8) | data[1];
    channels[5] = (data[4] << 8) | data[3];
    channels[6] = (data[6] << 8) | data[5];
    channels[7] = (data[8] << 8) | data[7];

    // Clear and NIR are available in both configurations
    channels[8] = (data[10] << 8) | data[9];  // Clear
    channels[9] = (data[12] << 8) | data[11]; // NIR

    return true;
}

/**
 * @brief Read a specific channel (415nm)
 * @param dev Pointer to AS7341 device structure
 * @param value Pointer to store the channel value
 * @return True if successful, false otherwise
 */
bool as7341_read_channel_415nm(as7341_t *dev, uint16_t *value) {
    if (!as7341_configure_f1_f4(dev)) {
        return false;
    }

    uint8_t data[2];
    if (!as7341_read_registers(dev, AS7341_CH0_DATA_L, data, 2)) {
        return false;
    }

    *value = (data[1] << 8) | data[0];
    return true;
}

// Similar functions for other channels (445nm, 480nm, etc.) would follow the same pattern

/**
 * @brief Set LED current
 * @param dev Pointer to AS7341 device structure
 * @param current_mA LED current in mA (4-258, even numbers only)
 * @return True if successful, false otherwise
 */
bool as7341_set_led_current(as7341_t *dev, uint16_t current_mA) {
    if (current_mA < 4 || current_mA > 258) {
        return false;
    }

    // Convert to register value (even numbers only)
    uint8_t reg_value = (current_mA - 4) / 2;

    // Switch to low bank
    if (!as7341_set_bank(dev, true)) {
        return false;
    }

    bool result = as7341_write_register(dev, AS7341_LED, reg_value);

    // Switch back to high bank
    as7341_set_bank(dev, false);

    return result;
}

/**
 * @brief Enable or disable LED
 * @param dev Pointer to AS7341 device structure
 * @param enable True to enable LED, false to disable
 * @return True if successful, false otherwise
 */
bool as7341_set_led(as7341_t *dev, bool enable) {
    // Switch to low bank
    if (!as7341_set_bank(dev, true)) {
        return false;
    }

    // Read current LED register value
    uint8_t reg_value;
    if (!as7341_read_register(dev, AS7341_LED, &reg_value)) {
        return false;
    }

    // Set or clear bit 7
    if (enable) {
        reg_value |= 0x80;
    } else {
        reg_value &= ~0x80;
    }

    bool result = as7341_write_register(dev, AS7341_LED, reg_value);

    // Switch back to high bank
    as7341_set_bank(dev, false);

    return result;
}

// Private functions

static bool as7341_write_register(as7341_t *dev, uint8_t reg, uint8_t value) {
    dev->buffer[0] = reg;
    dev->buffer[1] = value;

    if (HAL_I2C_Master_Transmit(dev->hi2c, dev->address << 1, dev->buffer, 2, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    return true;
}

static bool as7341_read_register(as7341_t *dev, uint8_t reg, uint8_t *value) {
    if (HAL_I2C_Master_Transmit(dev->hi2c, dev->address << 1, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    if (HAL_I2C_Master_Receive(dev->hi2c, dev->address << 1, value, 1, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    return true;
}

static bool as7341_read_registers(as7341_t *dev, uint8_t reg, uint8_t *data, uint8_t length) {
    if (HAL_I2C_Master_Transmit(dev->hi2c, dev->address << 1, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    if (HAL_I2C_Master_Receive(dev->hi2c, dev->address << 1, data, length, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    return true;
}

static bool as7341_set_smux(as7341_t *dev, smux_in_t smux_addr, smux_out_t smux_out1, smux_out_t smux_out2) {
    uint8_t low_nibble = smux_out1;
    uint8_t high_nibble = smux_out2 << 4;
    uint8_t smux_byte = high_nibble | low_nibble;
    return as7341_write_register(dev, smux_addr, smux_byte);
}

static void as7341_f1f4_clear_nir(as7341_t *dev) {
    as7341_set_smux(dev, SMUX_IN_NC_F3L, SMUX_OUT_DISABLED, SMUX_OUT_ADC2);
    as7341_set_smux(dev, SMUX_IN_F1L_NC, SMUX_OUT_ADC0, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_NC0, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_F8L, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F6L_NC, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F2L_F4L, SMUX_OUT_ADC1, SMUX_OUT_ADC3);
    as7341_set_smux(dev, SMUX_IN_NC_F5L, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F7L_NC, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_CL, SMUX_OUT_DISABLED, SMUX_OUT_ADC4);
    as7341_set_smux(dev, SMUX_IN_NC_F5R, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F7R_NC, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_NC1, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_F2R, SMUX_OUT_DISABLED, SMUX_OUT_ADC1);
    as7341_set_smux(dev, SMUX_IN_F4R_NC, SMUX_OUT_ADC3, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F8R_F6R, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_F3R, SMUX_OUT_DISABLED, SMUX_OUT_ADC2);
    as7341_set_smux(dev, SMUX_IN_F1R_EXT_GPIO, SMUX_OUT_ADC0, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_EXT_INT_CR, SMUX_OUT_DISABLED, SMUX_OUT_ADC4);
    as7341_set_smux(dev, SMUX_IN_NC_DARK, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NIR_F, SMUX_OUT_ADC5, SMUX_OUT_DISABLED);
}

static void as7341_f5f8_clear_nir(as7341_t *dev) {
    as7341_set_smux(dev, SMUX_IN_NC_F3L, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F1L_NC, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_NC0, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_F8L, SMUX_OUT_DISABLED, SMUX_OUT_ADC3);
    as7341_set_smux(dev, SMUX_IN_F6L_NC, SMUX_OUT_ADC1, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F2L_F4L, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_F5L, SMUX_OUT_DISABLED, SMUX_OUT_ADC0);
    as7341_set_smux(dev, SMUX_IN_F7L_NC, SMUX_OUT_ADC2, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_CL, SMUX_OUT_DISABLED, SMUX_OUT_ADC4);
    as7341_set_smux(dev, SMUX_IN_NC_F5R, SMUX_OUT_DISABLED, SMUX_OUT_ADC0);
    as7341_set_smux(dev, SMUX_IN_F7R_NC, SMUX_OUT_ADC2, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_NC1, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NC_F2R, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F4R_NC, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F8R_F6R, SMUX_OUT_ADC3, SMUX_OUT_ADC1);
    as7341_set_smux(dev, SMUX_IN_NC_F3R, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_F1R_EXT_GPIO, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_EXT_INT_CR, SMUX_OUT_DISABLED, SMUX_OUT_ADC4);
    as7341_set_smux(dev, SMUX_IN_NC_DARK, SMUX_OUT_DISABLED, SMUX_OUT_DISABLED);
    as7341_set_smux(dev, SMUX_IN_NIR_F, SMUX_OUT_ADC5, SMUX_OUT_DISABLED);
}

static bool as7341_wait_for_data(as7341_t *dev, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    uint8_t status;

    do {
        if (!as7341_read_register(dev, AS7341_STATUS2, &status)) {
            return false;
        }

        if (status & 0x40) { // Check data ready bit
            return true;
        }

        HAL_Delay(1);
    } while (HAL_GetTick() - start < timeout);

    return false;
}

static bool as7341_set_bank(as7341_t *dev, bool low_bank) {
    uint8_t cfg0;
    if (!as7341_read_register(dev, AS7341_CFG0, &cfg0)) {
        return false;
    }

    if (low_bank) {
        cfg0 |= 0x10; // Set bit 4
    } else {
        cfg0 &= ~0x10; // Clear bit 4
    }

    return as7341_write_register(dev, AS7341_CFG0, cfg0);
}
