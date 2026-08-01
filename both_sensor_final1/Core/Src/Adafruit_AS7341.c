/*
 * Adafruit_AS7341.c
 *
 *  Created on: Jul 29, 2026
 *      Author: HP
 */


/*!
 *  @file Adafruit_AS7341.c
 *
 * 	I2C Driver for the Library for the AS7341 11-Channel Spectral Sensor
 *
 * 	This is a library for the Adafruit AS7341 breakout:
 * 	https://www.adafruit.com/product/4698
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *  Copyright 2020 Bryan Siepert for Adafruit Industries
 *
 * 	BSD (see license.txt)
 */

/* Update by P. Chwalek to be compatible for STM32 C++ project (non-Arduino) */
/* Converted to C by request */

#include "Adafruit_AS7341.h"
#include "cmsis_os2.h"
//#include "captivate_config.h"

//extern osSemaphoreId_t messageI2C1_LockHandle;


/**
 * @brief Initialize the AS7341 device structure
 * @param dev Pointer to the device structure
 */
void Adafruit_AS7341_Init(Adafruit_AS7341_t *dev) {
    if (dev) {
        dev->i2c_han = NULL;
        dev->i2c_addr = 0;
        dev->last_spectral_int_source = 0;
        dev->_readingState = AS7341_WAITING_DONE;
        for (int i = 0; i < 12; i++) {
            dev->_channel_readings[i] = 0;
        }
    }
}

/**
 * @brief Sets up the hardware and initializes I2C
 * @param dev Pointer to the device structure
 * @param i2c_address The I2C address to be used
 * @param i2c_handle The I2C handle to be used
 * @param sensor_id The unique ID to differentiate the sensors from others
 * @return True if initialization was successful, otherwise false
 */
bool Adafruit_AS7341_begin(Adafruit_AS7341_t *dev, uint8_t i2c_address,
                           I2C_HandleTypeDef *i2c_handle, int32_t sensor_id) {
    if (!dev || !i2c_handle) return false;

    dev->i2c_han = i2c_handle;
    dev->i2c_addr = i2c_address << 1;

    return Adafruit_AS7341__init(dev, sensor_id);
}

/**
 * @brief Initializer for post i2c/spi init
 * @param dev Pointer to the device structure
 * @param sensor_id Optional unique ID for the sensor set
 * @returns True if chip identified and initialized
 */
bool Adafruit_AS7341__init(Adafruit_AS7341_t *dev, int32_t sensor_id) {
    (void)sensor_id; // Suppress unused parameter warning

    if (!dev) return false;

    // make sure we're talking to the right chip
    if ((Adafruit_AS7341_readRegisterByte(dev, AS7341_WHOAMI) & 0xFC) != (AS7341_CHIP_ID << 2)) {
        return false;
    }

    Adafruit_AS7341_powerEnable(dev, true);
    return true;
}

/********************* EXAMPLE EXTRACTS **************/
/**
 * @brief Returns the flicker detection status
 * @param dev Pointer to the device structure
 * @return int8_t
 */
int8_t Adafruit_AS7341_getFlickerDetectStatus(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    return (int8_t) Adafruit_AS7341_readRegisterByte(dev, AS7341_FD_STATUS);
}

/**
 * @brief Returns the ADC data for a given channel
 * @param dev Pointer to the device structure
 * @param channel The ADC channel to read
 * @return uint16_t The measured data for the currently configured sensor
 */
uint16_t Adafruit_AS7341_readChannel(Adafruit_AS7341_t *dev, as7341_adc_channel_t channel) {
    if (!dev) return 0;

    uint8_t data[2];
    Adafruit_AS7341_readRegister(dev, (uint16_t)(AS7341_CH0_DATA_L + 2 * channel), data, 2);
    return (((uint16_t)data[1]) << 8) | data[0];
}

/**
 * @brief Returns the reading data for the specified color channel
 * @param dev Pointer to the device structure
 * @param channel The color sensor channel to read
 * @return uint16_t The measured data for the selected sensor channel
 */
uint16_t Adafruit_AS7341_getChannel(Adafruit_AS7341_t *dev, as7341_color_channel_t channel) {
    if (!dev) return 0;
    return dev->_channel_readings[channel];
}

/**
 * @brief fills the provided buffer with the current measurements for Spectral
 * channels F1-8, Clear and NIR
 * @param dev Pointer to the device structure
 * @param readings_buffer Pointer to a buffer of length 10 or more to fill with sensor data
 * @return true: success false: failure
 */
bool Adafruit_AS7341_readAllChannels(Adafruit_AS7341_t *dev, uint16_t *readings_buffer) {
    if (!dev || !readings_buffer) return false;

    Adafruit_AS7341_setSMUXLowChannels(dev, true);        // Configure SMUX to read low channels
    Adafruit_AS7341_enableSpectralMeasurement(dev, true); // Start integration
    Adafruit_AS7341_delayForData(dev, 0);                 // I'll wait for you for all time

    bool low_success = Adafruit_AS7341_readRegister(dev, AS7341_CH0_DATA_L, (uint8_t*)readings_buffer, 12);

    Adafruit_AS7341_setSMUXLowChannels(dev, false);       // Configure SMUX to read high channels
    Adafruit_AS7341_enableSpectralMeasurement(dev, true); // Start integration
    Adafruit_AS7341_delayForData(dev, 0);                 // I'll wait for you for all time

    return low_success &&
           Adafruit_AS7341_readRegister(dev, AS7341_CH0_DATA_L, (uint8_t*)&readings_buffer[6], 12);
}

/**
 * @brief starts the process of getting readings from all channels without using delays
 * @param dev Pointer to the device structure
 * @return true: success false: failure
 */
bool Adafruit_AS7341_startReading(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    dev->_readingState = AS7341_WAITING_START; // Start the measurement please
    Adafruit_AS7341_checkReadingProgress(dev); // Call the check function to start it
    return true;
}

/**
 * @brief runs the process of getting readings from all channels without using delays
 * @param dev Pointer to the device structure
 * @return true: reading is complete false: reading is incomplete (or failed)
 */
bool Adafruit_AS7341_checkReadingProgress(Adafruit_AS7341_t *dev) {
    if (!dev) return false;

    if (dev->_readingState == AS7341_WAITING_START) {
        Adafruit_AS7341_setSMUXLowChannels(dev, true);        // Configure SMUX to read low channels
        Adafruit_AS7341_enableSpectralMeasurement(dev, true); // Start integration
        dev->_readingState = AS7341_WAITING_LOW;
        return false;
    }

    if (!Adafruit_AS7341_getIsDataReady(dev) || dev->_readingState == AS7341_WAITING_DONE)
        return false;

    if (dev->_readingState == AS7341_WAITING_LOW) {
        Adafruit_AS7341_readRegister(dev, AS7341_CH0_DATA_L, (uint8_t*)dev->_channel_readings, 12);

        Adafruit_AS7341_setSMUXLowChannels(dev, false);       // Configure SMUX to read high channels
        Adafruit_AS7341_enableSpectralMeasurement(dev, true); // Start integration
        dev->_readingState = AS7341_WAITING_HIGH;
        return false;
    }

    if (dev->_readingState == AS7341_WAITING_HIGH) {
        dev->_readingState = AS7341_WAITING_DONE;
        Adafruit_AS7341_readRegister(dev, AS7341_CH0_DATA_L, (uint8_t*)&dev->_channel_readings[6], 12);
        return true;
    }

    return false;
}

/**
 * @brief transfer all the values from the private result buffer into one nominated
 * @param dev Pointer to the device structure
 * @param readings_buffer Pointer to a buffer of length 12 (THERE IS NO ERROR CHECKING, YE BE WARNED!)
 * @return true: success false: failure
 */
bool Adafruit_AS7341_getAllChannels(Adafruit_AS7341_t *dev, uint32_t *readings_buffer) {
    if (!dev || !readings_buffer) return false;
    for (int i = 0; i < 12; i++)
        readings_buffer[i] = dev->_channel_readings[i];
    return true;
}

/**
 * @brief Delay while waiting for data, with option to time out and recover
 * @param dev Pointer to the device structure
 * @param waitTime the maximum amount of time to wait
 */
void Adafruit_AS7341_delayForData(Adafruit_AS7341_t *dev, uint32_t waitTime) {
    if (!dev) return;

    if (waitTime == 0) { // Wait forever
        while (!Adafruit_AS7341_getIsDataReady(dev)) {
//            osSemaphoreRelease(messageI2C1_LockHandle);
//            osDelay(1); // SF 2020-08-12 Does this really need to be so long?
//            osSemaphoreAcquire(messageI2C1_LockHandle, osWaitForever);
        }
        return;
    }
    if (waitTime > 0) { // Wait for that many milliseconds
        uint32_t elapsedMillis = 0;
        while ((!Adafruit_AS7341_getIsDataReady(dev)) && (elapsedMillis < waitTime)) {
//            osSemaphoreRelease(messageI2C1_LockHandle);
//            osDelay(1); // SF 2020-08-12 Does this really need to be so long?
//            osSemaphoreAcquire(messageI2C1_LockHandle, osWaitForever);
            elapsedMillis++;
        }
        return;
    }
    // if waitTime < 0, return immediately
}

/**
 * @brief Take readings for F1-8, Clear and NIR and store them in the internal buffer
 * @param dev Pointer to the device structure
 * @return true: success false: failure
 */
bool Adafruit_AS7341_readAllChannelsToBuffer(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return Adafruit_AS7341_readAllChannels(dev, dev->_channel_readings);
}

void Adafruit_AS7341_setSMUXLowChannels(Adafruit_AS7341_t *dev, bool f1_f4) {
    if (!dev) return;

    Adafruit_AS7341_enableSpectralMeasurement(dev, false);
    Adafruit_AS7341_setSMUXCommand(dev, AS7341_SMUX_CMD_WRITE);
    if (f1_f4) {
        Adafruit_AS7341_setup_F1F4_Clear_NIR(dev);
    } else {
        Adafruit_AS7341_setup_F5F8_Clear_NIR(dev);
    }
    Adafruit_AS7341_enableSMUX(dev);
}

/**
 * @brief Sets the power state of the sensor
 * @param dev Pointer to the device structure
 * @param enable_power true: on false: off
 */
void Adafruit_AS7341_powerEnable(Adafruit_AS7341_t *dev, bool enable_power) {
    if (!dev) return;
    uint8_t enable_reg = Adafruit_AS7341_readRegisterByte(dev, AS7341_ENABLE);
    enable_reg = Adafruit_AS7341_modifyBitInByte(enable_reg, (uint8_t)enable_power, 0);
    Adafruit_AS7341_writeRegisterByte(dev, AS7341_ENABLE, enable_reg);
}

/**
 * @brief Disable Spectral reading, flicker detection, and power
 * @param dev Pointer to the device structure
 */
void Adafruit_AS7341_disableAll(Adafruit_AS7341_t *dev) {
    if (!dev) return;
    Adafruit_AS7341_writeRegisterByte(dev, AS7341_ENABLE, 0);
}

/**
 * @brief Enables measurement of spectral data
 * @param dev Pointer to the device structure
 * @param enable_measurement true: enabled false: disabled
 * @return true: success false: failure
 */
bool Adafruit_AS7341_enableSpectralMeasurement(Adafruit_AS7341_t *dev, bool enable_measurement) {
    if (!dev) return false;

    uint8_t enable_reg = Adafruit_AS7341_readRegisterByte(dev, AS7341_ENABLE);
    enable_reg = Adafruit_AS7341_modifyBitInByte(enable_reg, (uint8_t)enable_measurement, 1);

    return Adafruit_AS7341_writeRegisterByte(dev, AS7341_ENABLE, enable_reg);
}

bool Adafruit_AS7341_enableSMUX(Adafruit_AS7341_t *dev) {
    if (!dev) return false;

    bool success = Adafruit_AS7341_modifyRegisterBit(dev, AS7341_ENABLE, true, 4);

    int timeOut = 1000; // Arbitrary value
    int count = 0;
    while (Adafruit_AS7341_checkRegisterBit(dev, AS7341_ENABLE, 4) && count < timeOut) {
//        osSemaphoreRelease(messageI2C1_LockHandle);
//        osDelay(1);
//        osSemaphoreAcquire(messageI2C1_LockHandle, osWaitForever);
        count++;
    }
    if (count >= timeOut)
        return false;
    else
        return success;
}

bool Adafruit_AS7341_enableFlickerDetection(Adafruit_AS7341_t *dev, bool enable_fd) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_ENABLE, enable_fd, 6);
}

/**
 * @brief Get the GPIO pin direction setting
 * @param dev Pointer to the device structure
 * @return `AS7341_OUTPUT` or `AS7341_INPUT`
 */
as7341_gpio_dir_t Adafruit_AS7341_getGPIODirection(Adafruit_AS7341_t *dev) {
    if (!dev) return AS7341_GPIO_INPUT;
    return (as7341_gpio_dir_t)Adafruit_AS7341_checkRegisterBit(dev, AS7341_GPIO2, 2);
}

/**
 * @brief Set the GPIO pin to be used as an input or output
 * @param dev Pointer to the device structure
 * @param gpio_direction The IO direction to set
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setGPIODirection(Adafruit_AS7341_t *dev, as7341_gpio_dir_t gpio_direction) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_GPIO2, gpio_direction, 2);
}

/**
 * @brief Get the output inversion setting for the GPIO pin
 * @param dev Pointer to the device structure
 * @return true: GPIO output inverted false: GPIO output normal
 */
bool Adafruit_AS7341_getGPIOInverted(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return Adafruit_AS7341_checkRegisterBit(dev, AS7341_GPIO2, 3);
}

/**
 * @brief Invert the logic of the GPIO pin when used as an output
 * @param dev Pointer to the device structure
 * @param gpio_inverted When true setting the gpio value to true will connect the GPIO pin to ground
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setGPIOInverted(Adafruit_AS7341_t *dev, bool gpio_inverted) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_GPIO2, gpio_inverted, 3);
}

/**
 * @brief Read the digital level of the GPIO pin
 * @param dev Pointer to the device structure
 * @return true: GPIO pin level is high false: GPIO pin level is low
 */
bool Adafruit_AS7341_getGPIOValue(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return Adafruit_AS7341_checkRegisterBit(dev, AS7341_GPIO2, 0);
}

/**
 * @brief Set the digital level of the GPIO pin
 * @param dev Pointer to the device structure
 * @param gpio_high The GPIO level to set
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setGPIOValue(Adafruit_AS7341_t *dev, bool gpio_high) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_GPIO2, gpio_high, 1);
}

bool Adafruit_AS7341_setSMUXCommand(Adafruit_AS7341_t *dev, as7341_smux_cmd_t command) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterMultipleBit(dev, AS7341_CFG6, command, 3, 2);
}

/**
 * @brief Enable control of an attached LED on the LDR pin
 * @param dev Pointer to the device structure
 * @param enable_led true: LED enabled false: LED disabled
 * @return true: success false: failure
 */
bool Adafruit_AS7341_enableLED(Adafruit_AS7341_t *dev, bool enable_led) {
    if (!dev) return false;

    Adafruit_AS7341_setBank(dev, true); // Access 0x60-0x74
    bool result = Adafruit_AS7341_modifyRegisterBit(dev, AS7341_CONFIG, enable_led, 3) &&
                  Adafruit_AS7341_modifyRegisterBit(dev, AS7341_LED, enable_led, 7);
    Adafruit_AS7341_setBank(dev, false); // Access registers 0x80 and above (default)
    return result;
}

/**
 * @brief Set the current limit for the LED
 * @param dev Pointer to the device structure
 * @param led_current_ma the value to set in milliamps
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setLEDCurrent(Adafruit_AS7341_t *dev, uint16_t led_current_ma) {
    if (!dev) return false;

    if (led_current_ma > 258) {
        return false;
    }
    if (led_current_ma < 4) {
        led_current_ma = 4;
    }

    Adafruit_AS7341_setBank(dev, true); // Access 0x60 0x74
    bool result = Adafruit_AS7341_modifyRegisterMultipleBit(dev, AS7341_LED,
                     (uint8_t)((led_current_ma - 4) / 2), 0, 7);
    Adafruit_AS7341_setBank(dev, false); // Access registers 0x80 and above (default)
    return result;
}

/**
 * @brief Sets the active register bank
 * @param dev Pointer to the device structure
 * @param low true: access to registers 0x60-0x74, false: access to registers 0x80 and above
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setBank(Adafruit_AS7341_t *dev, bool low) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_CFG0, low, 4);
}

/**
 * @brief Sets the threshold below which spectral measurements will trigger interrupts
 * @param dev Pointer to the device structure
 * @param low_threshold the new threshold
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setLowThreshold(Adafruit_AS7341_t *dev, uint16_t low_threshold) {
    if (!dev) return false;
    return Adafruit_AS7341_writeRegister(dev, AS7341_SP_LOW_TH_L, (uint8_t*)&low_threshold, 2);
}

/**
 * @brief Returns the current low threshold for spectral measurements
 * @param dev Pointer to the device structure
 * @return uint16_t The current low threshold
 */
uint16_t Adafruit_AS7341_getLowThreshold(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    uint8_t data[2];
    Adafruit_AS7341_readRegister(dev, AS7341_SP_LOW_TH_L, data, 2);
    return (((uint16_t)data[1]) << 8) | data[0];
}

/**
 * @brief Sets the threshold above which spectral measurements will trigger interrupts
 * @param dev Pointer to the device structure
 * @param high_threshold the new threshold
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setHighThreshold(Adafruit_AS7341_t *dev, uint16_t high_threshold) {
    if (!dev) return false;
    return Adafruit_AS7341_writeRegister(dev, AS7341_SP_HIGH_TH_L, (uint8_t*)&high_threshold, 2);
}

/**
 * @brief Returns the current high threshold for spectral measurements
 * @param dev Pointer to the device structure
 * @return uint16_t The current high threshold
 */
uint16_t Adafruit_AS7341_getHighThreshold(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    uint8_t data[2];
    Adafruit_AS7341_readRegister(dev, AS7341_SP_HIGH_TH_L, data, 2);
    return (((uint16_t)data[1]) << 8) | data[0];
}

/**
 * @brief Enable Interrupts based on spectral measurements
 * @param dev Pointer to the device structure
 * @param enable_int true: enable false: disable
 * @return true: success false: failure
 */
bool Adafruit_AS7341_enableSpectralInterrupt(Adafruit_AS7341_t *dev, bool enable_int) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_INTENAB, enable_int, 3);
}

/**
 * @brief Enable system interrupts
 * @param dev Pointer to the device structure
 * @param enable_int true: enable false: disable
 * @return true: success false: failure
 */
bool Adafruit_AS7341_enableSystemInterrupt(Adafruit_AS7341_t *dev, bool enable_int) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterBit(dev, AS7341_INTENAB, enable_int, 0);
}

/**
 * @brief Sets the number of times an interrupt threshold must be exceeded before an interrupt is triggered
 * @param dev Pointer to the device structure
 * @param cycle_count The number of cycles to trigger an interrupt
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setAPERS(Adafruit_AS7341_t *dev, as7341_int_cycle_count_t cycle_count) {
    if (!dev) return false;
    return Adafruit_AS7341_modifyRegisterMultipleBit(dev, AS7341_PERS, cycle_count, 0, 4);
}

/**
 * @brief Set the ADC channel to use for spectral thresholds
 * @param dev Pointer to the device structure
 * @param channel The channel to use for spectral thresholds
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setSpectralThresholdChannel(Adafruit_AS7341_t *dev, as7341_adc_channel_t channel) {
    if (!dev) return false;
    if (channel == AS7341_ADC_CHANNEL_5) {
        return false;
    }
    return Adafruit_AS7341_modifyRegisterMultipleBit(dev, AS7341_CFG12, channel, 0, 3);
}

/**
 * @brief Returns the current value of the Interrupt status register
 * @param dev Pointer to the device structure
 * @return uint8_t
 */
uint8_t Adafruit_AS7341_getInterruptStatus(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    return (uint8_t)Adafruit_AS7341_readRegisterByte(dev, AS7341_STATUS);
}

/**
 * @brief Returns the status of the spectral measurement threshold interrupts
 * @param dev Pointer to the device structure
 * @return true: interrupt triggered false: interrupt not triggered
 */
bool Adafruit_AS7341_spectralInterruptTriggered(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return Adafruit_AS7341_checkRegisterBit(dev, AS7341_STATUS, 3);
}

/**
 * @brief Clear the interrupt status register
 * @param dev Pointer to the device structure
 * @return true: success false: failure
 */
bool Adafruit_AS7341_clearInterruptStatus(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return Adafruit_AS7341_writeRegisterByte(dev, AS7341_STATUS, 0xFF);
}

/**
 * @brief Get the current state of the spectral measurement interrupt status register
 * @param dev Pointer to the device structure
 * @return uint8_t The current status register
 */
uint8_t Adafruit_AS7341_spectralInterruptSource(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    uint8_t spectral_int_source = Adafruit_AS7341_readRegisterByte(dev, AS7341_STATUS3);
    dev->last_spectral_int_source = spectral_int_source;
    return spectral_int_source;
}

/**
 * @brief The status of the low threshold interrupt
 * @param dev Pointer to the device structure
 * @return true: low interrupt triggered false: interrupt not triggered
 */
bool Adafruit_AS7341_spectralLowTriggered(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return (dev->last_spectral_int_source & AS7341_SPECTRAL_INT_LOW_MSK) > 0;
}

/**
 * @brief The status of the high threshold interrupt
 * @param dev Pointer to the device structure
 * @return true: high interrupt triggered false: interrupt not triggered
 */
bool Adafruit_AS7341_spectralHighTriggered(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return (dev->last_spectral_int_source & AS7341_SPECTRAL_INT_HIGH_MSK) > 0;
}

/**
 * @brief Check if data is ready
 * @param dev Pointer to the device structure
 * @return true: data ready false: data not ready
 */
bool Adafruit_AS7341_getIsDataReady(Adafruit_AS7341_t *dev) {
    if (!dev) return false;
    return Adafruit_AS7341_checkRegisterBit(dev, AS7341_STATUS2, 6);
}

/**
 * @brief Configure SMUX for sensors F1-4, Clear and NIR
 * @param dev Pointer to the device structure
 */
void Adafruit_AS7341_setup_F1F4_Clear_NIR(Adafruit_AS7341_t *dev) {
    if (!dev) return;
    // SMUX Config for F1,F2,F3,F4,NIR,Clear
    Adafruit_AS7341_writeRegisterByte(dev, 0x00, 0x30); // F3 left set to ADC2
    Adafruit_AS7341_writeRegisterByte(dev, 0x01, 0x01); // F1 left set to ADC0
    Adafruit_AS7341_writeRegisterByte(dev, 0x02, 0x00); // Reserved or disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x03, 0x00); // F8 left disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x04, 0x00); // F6 left disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x05, 0x42); // F4 left connected to ADC3/f2 left connected to ADC1
    Adafruit_AS7341_writeRegisterByte(dev, 0x06, 0x00); // F5 left disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x07, 0x00); // F7 left disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x08, 0x50); // CLEAR connected to ADC4
    Adafruit_AS7341_writeRegisterByte(dev, 0x09, 0x00); // F5 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0A, 0x00); // F7 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0B, 0x00); // Reserved or disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0C, 0x20); // F2 right connected to ADC1
    Adafruit_AS7341_writeRegisterByte(dev, 0x0D, 0x04); // F4 right connected to ADC3
    Adafruit_AS7341_writeRegisterByte(dev, 0x0E, 0x00); // F6/F8 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0F, 0x30); // F3 right connected to AD2
    Adafruit_AS7341_writeRegisterByte(dev, 0x10, 0x01); // F1 right connected to AD0
    Adafruit_AS7341_writeRegisterByte(dev, 0x11, 0x50); // CLEAR right connected to AD4
    Adafruit_AS7341_writeRegisterByte(dev, 0x12, 0x00); // Reserved or disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x13, 0x06); // NIR connected to ADC5
}

/**
 * @brief Configure SMUX for sensors F5-8, Clear and NIR
 * @param dev Pointer to the device structure
 */
void Adafruit_AS7341_setup_F5F8_Clear_NIR(Adafruit_AS7341_t *dev) {
    if (!dev) return;
    // SMUX Config for F5,F6,F7,F8,NIR,Clear
    Adafruit_AS7341_writeRegisterByte(dev, 0x00, 0x00); // F3 left disable
    Adafruit_AS7341_writeRegisterByte(dev, 0x01, 0x00); // F1 left disable
    Adafruit_AS7341_writeRegisterByte(dev, 0x02, 0x00); // reserved/disable
    Adafruit_AS7341_writeRegisterByte(dev, 0x03, 0x40); // F8 left connected to ADC3
    Adafruit_AS7341_writeRegisterByte(dev, 0x04, 0x02); // F6 left connected to ADC1
    Adafruit_AS7341_writeRegisterByte(dev, 0x05, 0x00); // F4/ F2 disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x06, 0x10); // F5 left connected to ADC0
    Adafruit_AS7341_writeRegisterByte(dev, 0x07, 0x03); // F7 left connected to ADC2
    Adafruit_AS7341_writeRegisterByte(dev, 0x08, 0x50); // CLEAR Connected to ADC4
    Adafruit_AS7341_writeRegisterByte(dev, 0x09, 0x10); // F5 right connected to ADC0
    Adafruit_AS7341_writeRegisterByte(dev, 0x0A, 0x03); // F7 right connected to ADC2
    Adafruit_AS7341_writeRegisterByte(dev, 0x0B, 0x00); // Reserved or disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0C, 0x00); // F2 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0D, 0x00); // F4 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x0E, 0x24); // F8 right connected to ADC2/ F6 right connected to ADC1
    Adafruit_AS7341_writeRegisterByte(dev, 0x0F, 0x00); // F3 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x10, 0x00); // F1 right disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x11, 0x50); // CLEAR right connected to AD4
    Adafruit_AS7341_writeRegisterByte(dev, 0x12, 0x00); // Reserved or disabled
    Adafruit_AS7341_writeRegisterByte(dev, 0x13, 0x06); // NIR connected to ADC5
}

/**
 * @brief Configure SMUX for flicker detection
 * @param dev Pointer to the device structure
 */
void Adafruit_AS7341_FDConfig(Adafruit_AS7341_t *dev) {
    if (!dev) return;
    // SMUX Config for Flicker- register (0x13)left set to ADC6 for flicker detection
    Adafruit_AS7341_writeRegisterByte(dev, 0x00, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x01, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x02, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x03, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x04, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x05, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x06, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x07, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x08, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x09, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x0A, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x0B, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x0C, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x0D, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x0E, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x0F, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x10, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x11, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x12, 0x00);
    Adafruit_AS7341_writeRegisterByte(dev, 0x13, 0x60); // Flicker connected to ADC5 to left of 0x13
}

/**
 * @brief Sets the integration time step count
 * @param dev Pointer to the device structure
 * @param atime_value The integration time step count
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setATIME(Adafruit_AS7341_t *dev, uint8_t atime_value) {
    if (!dev) return false;
    return Adafruit_AS7341_writeRegisterByte(dev, AS7341_ATIME, atime_value);
}

/**
 * @brief Returns the integration time step count
 * @param dev Pointer to the device structure
 * @return uint8_t The current integration time step count
 */
uint8_t Adafruit_AS7341_getATIME(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    return Adafruit_AS7341_readRegisterByte(dev, AS7341_ATIME);
}

/**
 * @brief Sets the integration time step size
 * @param dev Pointer to the device structure
 * @param astep_value Integration time step size in 2.78 microsecond increments
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setASTEP(Adafruit_AS7341_t *dev, uint16_t astep_value) {
    if (!dev) return false;
    return Adafruit_AS7341_writeRegister(dev, AS7341_ASTEP_L, (uint8_t*)&astep_value, 2);
}

/**
 * @brief Returns the integration time step size
 * @param dev Pointer to the device structure
 * @return uint16_t The current integration time step size
 */
uint16_t Adafruit_AS7341_getASTEP(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    uint8_t data[2];
    Adafruit_AS7341_readRegister(dev, AS7341_ASTEP_L, data, 2);
    return (((uint16_t)data[1]) << 8) | data[0];
}

/**
 * @brief Sets the ADC gain multiplier
 * @param dev Pointer to the device structure
 * @param gain_value The gain amount
 * @return true: success false: failure
 */
bool Adafruit_AS7341_setGain(Adafruit_AS7341_t *dev, as7341_gain_t gain_value) {
    if (!dev) return false;
    return Adafruit_AS7341_writeRegisterByte(dev, AS7341_CFG1, gain_value);
}

/**
 * @brief Returns the ADC gain multiplier
 * @param dev Pointer to the device structure
 * @return as7341_gain_t The current ADC gain multiplier
 */
as7341_gain_t Adafruit_AS7341_getGain(Adafruit_AS7341_t *dev) {
    if (!dev) return AS7341_GAIN_1X;
    return (as7341_gain_t)Adafruit_AS7341_readRegisterByte(dev, AS7341_CFG1);
}

/**
 * @brief Returns the integration time
 * @param dev Pointer to the device structure
 * @return long The current integration time in ms
 */
long Adafruit_AS7341_getTINT(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;
    uint16_t astep = Adafruit_AS7341_getASTEP(dev);
    uint8_t atime = Adafruit_AS7341_getATIME(dev);

    return (atime + 1) * (astep + 1) * 278 / 100000; // 2.78us * 1000 / 1000000 * 1000 = 2.78/1000
}

/**
 * @brief Converts raw ADC values to basic counts
 * @param dev Pointer to the device structure
 * @param raw The raw ADC values to convert
 * @return float The basic counts
 */
/**
 * @brief Converts raw ADC values to basic counts
 * @param dev Pointer to the device structure
 * @param raw The raw ADC values to convert
 * @return float The basic counts
 */
float Adafruit_AS7341_toBasicCounts(Adafruit_AS7341_t *dev, uint16_t raw) {
    if (!dev) return 0.0f;

    float gain_val = 0;
    as7341_gain_t gain = Adafruit_AS7341_getGain(dev);
    switch (gain) {
    case AS7341_GAIN_0_5X: gain_val = 0.5f; break;
    case AS7341_GAIN_1X:   gain_val = 1.0f; break;
    case AS7341_GAIN_2X:   gain_val = 2.0f; break;
    case AS7341_GAIN_4X:   gain_val = 4.0f; break;
    case AS7341_GAIN_8X:   gain_val = 8.0f; break;
    case AS7341_GAIN_16X:  gain_val = 16.0f; break;
    case AS7341_GAIN_32X:  gain_val = 32.0f; break;
    case AS7341_GAIN_64X:  gain_val = 64.0f; break;
    case AS7341_GAIN_128X: gain_val = 128.0f; break;
    case AS7341_GAIN_256X: gain_val = 256.0f; break;
    case AS7341_GAIN_512X: gain_val = 512.0f; break;
    default: gain_val = 1.0f; break;
    }
    return raw / (gain_val * (Adafruit_AS7341_getATIME(dev) + 1) * (Adafruit_AS7341_getASTEP(dev) + 1) * 2.78f / 1000.0f);
}
/**
 * @brief Detect a flickering light
 * @param dev Pointer to the device structure
 * @return The frequency of a detected flicker or 1 if a flicker of unknown frequency is detected
 */
uint16_t Adafruit_AS7341_detectFlickerHz(Adafruit_AS7341_t *dev) {
    if (!dev) return 0;

    // disable everything
    Adafruit_AS7341_disableAll(dev);
    // re-enable power
    Adafruit_AS7341_powerEnable(dev, true);

    // Write SMUX configuration from RAM to set SMUX chain registers
    Adafruit_AS7341_setSMUXCommand(dev, AS7341_SMUX_CMD_WRITE);

    // Write new configuration to all the 20 registers for detecting Flicker
    Adafruit_AS7341_FDConfig(dev);

    // Start SMUX command
    Adafruit_AS7341_enableSMUX(dev);

    // Enable SP_EN bit
    Adafruit_AS7341_enableSpectralMeasurement(dev, true);

    // Enable flicker detection bit
    Adafruit_AS7341_writeRegisterByte(dev, AS7341_ENABLE, 0x41);

//    osSemaphoreRelease(messageI2C1_LockHandle);
//    osDelay(500);
//    osSemaphoreAcquire(messageI2C1_LockHandle, osWaitForever);

    uint16_t flicker_status = Adafruit_AS7341_getFlickerDetectStatus(dev);
    Adafruit_AS7341_enableFlickerDetection(dev, false);
    switch (flicker_status) {
    case 44: return 1;
    case 45: return 100;
    case 46: return 120;
    default: return 0;
    }
}

/* ============================================================ */
/*                      I2C Helper Functions                      */
/* ============================================================ */

/**
 * @brief Write multiple bytes to a register
 * @param dev Pointer to the device structure
 * @param mem_addr Register address
 * @param val Pointer to data to write
 * @param size Number of bytes to write
 * @return true: success false: failure
 */
bool Adafruit_AS7341_writeRegister(Adafruit_AS7341_t *dev, uint8_t mem_addr, uint8_t *val, uint16_t size) {
    if (!dev || !val || dev->i2c_han == NULL) return false;
    if (HAL_OK == HAL_I2C_Mem_Write(dev->i2c_han, dev->i2c_addr, mem_addr, 1, val, size, 10)) {
        return true;
    }
    return false;
}

/**
 * @brief Write a single byte to a register
 * @param dev Pointer to the device structure
 * @param mem_addr Register address
 * @param val Value to write
 * @return true: success false: failure
 */
bool Adafruit_AS7341_writeRegisterByte(Adafruit_AS7341_t *dev, uint8_t mem_addr, uint8_t val) {
    if (!dev || dev->i2c_han == NULL) return false;
    if (HAL_OK == HAL_I2C_Mem_Write(dev->i2c_han, dev->i2c_addr, mem_addr, 1, &val, 1, 10)) {
        return true;
    }
    return false;
}

/**
 * @brief Modify a single bit in a byte value
 * @param var The byte to modify
 * @param value The bit value (0 or 1)
 * @param pos The bit position (0-7)
 * @return The modified byte
 */
uint8_t Adafruit_AS7341_modifyBitInByte(uint8_t var, uint8_t value, uint8_t pos) {
    uint8_t mask = 1 << pos;
    return (var & ~mask) | ((value & 1) << pos);
}

/**
 * @brief Read multiple bytes from a register
 * @param dev Pointer to the device structure
 * @param mem_addr Register address
 * @param dest Pointer to destination buffer
 * @param size Number of bytes to read
 * @return true: success false: failure
 */
bool Adafruit_AS7341_readRegister(Adafruit_AS7341_t *dev, uint16_t mem_addr, uint8_t *dest, uint16_t size) {
    if (!dev || !dest || dev->i2c_han == NULL) return false;
    if (HAL_OK == HAL_I2C_Mem_Read(dev->i2c_han, dev->i2c_addr, mem_addr, 1, dest, size, 10)) {
        return true;
    }
    return false;
}

/**
 * @brief Check a specific bit in a register
 * @param dev Pointer to the device structure
 * @param reg Register address
 * @param pos Bit position (0-7)
 * @return The bit value (0 or 1)
 */
uint8_t Adafruit_AS7341_checkRegisterBit(Adafruit_AS7341_t *dev, uint16_t reg, uint8_t pos) {
    if (!dev) return 0;
    return (Adafruit_AS7341_readRegisterByte(dev, reg) >> pos) & 0x01;
}

/**
 * @brief Modify a specific bit in a register
 * @param dev Pointer to the device structure
 * @param reg Register address
 * @param value The bit value (true/false)
 * @param pos Bit position (0-7)
 * @return true: success false: failure
 */
bool Adafruit_AS7341_modifyRegisterBit(Adafruit_AS7341_t *dev, uint16_t reg, bool value, uint8_t pos) {
    if (!dev) return false;
    uint8_t register_value = Adafruit_AS7341_readRegisterByte(dev, reg);
    register_value = Adafruit_AS7341_modifyBitInByte(register_value, (uint8_t)value, pos);
    return Adafruit_AS7341_writeRegisterByte(dev, reg, register_value);
}

/**
 * @brief Modify multiple bits in a register
 * @param dev Pointer to the device structure
 * @param reg Register address
 * @param value The value to set
 * @param pos Starting bit position
 * @param bits Number of bits to modify
 * @return true: success false: failure
 */
bool Adafruit_AS7341_modifyRegisterMultipleBit(Adafruit_AS7341_t *dev, uint16_t reg, uint8_t value,
                                               uint8_t pos, uint8_t bits) {
    if (!dev) return false;
    uint8_t register_value = Adafruit_AS7341_readRegisterByte(dev, reg);

    uint8_t mask = (1 << bits) - 1;
    value &= mask;

    mask <<= pos;
    register_value &= ~mask;
    register_value |= value << pos;

    return Adafruit_AS7341_writeRegisterByte(dev, reg, register_value);
}

/**
 * @brief Read a single byte from a register
 * @param dev Pointer to the device structure
 * @param mem_addr Register address
 * @return The register value
 */
uint8_t Adafruit_AS7341_readRegisterByte(Adafruit_AS7341_t *dev, uint16_t mem_addr) {
    if (!dev || dev->i2c_han == NULL) return 0;
    uint8_t data = 0;
    HAL_I2C_Mem_Read(dev->i2c_han, dev->i2c_addr, mem_addr, 1, &data, 1, 10);
    return data;
}





bool Adafruit_AS7341_take10ChannelReadings(Adafruit_AS7341_t *dev,
                                           uint16_t *readings10)
{
    if (!dev || !readings10)
        return false;

    uint16_t raw[12];

    if (!Adafruit_AS7341_readAllChannels(dev, raw))
        return false;

    readings10[0] = raw[0];                       // F1
    readings10[1] = raw[1];                       // F2
    readings10[2] = raw[2];                       // F3
    readings10[3] = raw[3];                       // F4

    readings10[4] = raw[6];                       // F5
    readings10[5] = raw[7];                       // F6
    readings10[6] = raw[8];                       // F7
    readings10[7] = raw[9];                       // F8

    readings10[8] = (raw[4] + raw[10]) / 2;       // Average Clear
    readings10[9] = (raw[5] + raw[11]) / 2;       // Average NIR

    return true;
}
