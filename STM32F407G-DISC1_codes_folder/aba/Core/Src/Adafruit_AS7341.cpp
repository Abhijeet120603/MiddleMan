/*!
 *  @file Adafruit_AS7341.cpp
 *  HAL-only STM32 version (NO FreeRTOS)
 */

#include "Adafruit_AS7341.h"

/* ================= CONSTRUCTOR / DESTRUCTOR ================= */

Adafruit_AS7341::Adafruit_AS7341(void) {
}

Adafruit_AS7341::~Adafruit_AS7341(void) {
}

/* ================= INITIALIZATION ================= */

bool Adafruit_AS7341::begin(uint8_t i2c_address,
                            I2C_HandleTypeDef *i2c_handle,
                            int32_t sensor_id) {
    i2c_han = i2c_handle;
    i2c_addr = i2c_address << 1;
    return _init(sensor_id);
}

bool Adafruit_AS7341::_init(int32_t sensor_id) {
    if ((readRegisterByte(AS7341_WHOAMI) & 0xFC) != (AS7341_CHIP_ID << 2)) {
        return false;
    }
    powerEnable(true);
    return true;
}

/* ================= BASIC READ FUNCTIONS ================= */

int8_t Adafruit_AS7341::getFlickerDetectStatus(void) {
    return (int8_t)readRegisterByte(AS7341_FD_STATUS);
}

uint16_t Adafruit_AS7341::readChannel(as7341_adc_channel_t channel) {
    uint8_t data[2];
    readRegister(AS7341_CH0_DATA_L + 2 * channel, data, 2);
    return ((uint16_t)data[1] << 8) | data[0];
}

uint16_t Adafruit_AS7341::getChannel(as7341_color_channel_t channel) {
    return _channel_readings[channel];
}

/* ================= MULTI CHANNEL READ ================= */

bool Adafruit_AS7341::readAllChannels(uint16_t *readings_buffer) {

    setSMUXLowChannels(true);
    enableSpectralMeasurement(true);
    delayForData(0);

    bool low_ok = readRegister(AS7341_CH0_DATA_L,
                               (uint8_t *)readings_buffer,
                               12);

    setSMUXLowChannels(false);
    enableSpectralMeasurement(true);
    delayForData(0);

    return low_ok &&
           readRegister(AS7341_CH0_DATA_L,
                        (uint8_t *)&readings_buffer[6],
                        12);
}

bool Adafruit_AS7341::readAllChannels(void) {
    return readAllChannels(_channel_readings);
}

/* ================= NON-BLOCKING READ ================= */

bool Adafruit_AS7341::startReading(void) {
    _readingState = AS7341_WAITING_START;
    checkReadingProgress();
    return true;
}

bool Adafruit_AS7341::checkReadingProgress() {

    if (_readingState == AS7341_WAITING_START) {
        setSMUXLowChannels(true);
        enableSpectralMeasurement(true);
        _readingState = AS7341_WAITING_LOW;
        return false;
    }

    if (!getIsDataReady() || _readingState == AS7341_WAITING_DONE)
        return false;

    if (_readingState == AS7341_WAITING_LOW) {
        readRegister(AS7341_CH0_DATA_L,
                     (uint8_t *)_channel_readings, 12);
        setSMUXLowChannels(false);
        enableSpectralMeasurement(true);
        _readingState = AS7341_WAITING_HIGH;
        return false;
    }

    if (_readingState == AS7341_WAITING_HIGH) {
        readRegister(AS7341_CH0_DATA_L,
                     (uint8_t *)&_channel_readings[6], 12);
        _readingState = AS7341_WAITING_DONE;
        return true;
    }

    return false;
}

bool Adafruit_AS7341::getAllChannels(uint32_t *readings_buffer) {
    for (int i = 0; i < 12; i++)
        readings_buffer[i] = (uint32_t)_channel_readings[i];
    return true;
}

/* ================= DELAY HANDLING (HAL ONLY) ================= */

void Adafruit_AS7341::delayForData(uint32_t waitTime) {
    uint32_t start = HAL_GetTick();
    while (!getIsDataReady()) {
        if (waitTime > 0 && (HAL_GetTick() - start) >= waitTime)
            break;
        HAL_Delay(1);
    }
}

/* ================= POWER / ENABLE ================= */

void Adafruit_AS7341::powerEnable(bool enable_power) {
    uint8_t reg = readRegisterByte(AS7341_ENABLE);
    reg = modifyBitInByte(reg, enable_power, 0);
    writeRegisterByte(AS7341_ENABLE, reg);
}

void Adafruit_AS7341::disableAll(void) {
    writeRegisterByte(AS7341_ENABLE, 0);
}

bool Adafruit_AS7341::enableSpectralMeasurement(bool enable) {
    uint8_t reg = readRegisterByte(AS7341_ENABLE);
    reg = modifyBitInByte(reg, enable, 1);
    return writeRegisterByte(AS7341_ENABLE, reg);
}

bool Adafruit_AS7341::enableSMUX(void) {
    bool ok = modifyRegisterBit(AS7341_ENABLE, true, 4);
    uint32_t start = HAL_GetTick();
    while (checkRegisterBit(AS7341_ENABLE, 4)) {
        if (HAL_GetTick() - start > 1000)
            return false;
        HAL_Delay(1);
    }
    return ok;
}

/* ================= TIMING / GAIN ================= */

bool Adafruit_AS7341::setATIME(uint8_t atime) {
    return writeRegisterByte(AS7341_ATIME, atime);
}

uint8_t Adafruit_AS7341::getATIME() {
    return readRegisterByte(AS7341_ATIME);
}

bool Adafruit_AS7341::setASTEP(uint16_t astep) {
    return writeRegister(AS7341_ASTEP_L, (uint8_t *)&astep, 2);
}

uint16_t Adafruit_AS7341::getASTEP() {
    uint8_t d[2];
    readRegister(AS7341_ASTEP_L, d, 2);
    return ((uint16_t)d[1] << 8) | d[0];   // ✅ FIXED
}

bool Adafruit_AS7341::setGain(as7341_gain_t gain) {
    return writeRegisterByte(AS7341_CFG1, gain);
}

as7341_gain_t Adafruit_AS7341::getGain() {
    return (as7341_gain_t)readRegisterByte(AS7341_CFG1);
}

/* ================= I2C LOW LEVEL ================= */

bool Adafruit_AS7341::writeRegister(uint8_t reg, uint8_t *val, uint16_t size) {
    return HAL_I2C_Mem_Write(i2c_han, i2c_addr, reg,
                             I2C_MEMADD_SIZE_8BIT,
                             val, size, 100) == HAL_OK;
}

bool Adafruit_AS7341::writeRegisterByte(uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(i2c_han, i2c_addr, reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &val, 1, 100) == HAL_OK;
}

bool Adafruit_AS7341::readRegister(uint16_t reg, uint8_t *dest, uint16_t size) {
    return HAL_I2C_Mem_Read(i2c_han, i2c_addr, reg,
                            I2C_MEMADD_SIZE_8BIT,
                            dest, size, 100) == HAL_OK;
}

uint8_t Adafruit_AS7341::readRegisterByte(uint16_t reg) {
    uint8_t d = 0;
    HAL_I2C_Mem_Read(i2c_han, i2c_addr, reg,
                     I2C_MEMADD_SIZE_8BIT,
                     &d, 1, 100);
    return d;
}

/* ================= BIT HELPERS ================= */

uint8_t Adafruit_AS7341::modifyBitInByte(uint8_t var,
                                         uint8_t val,
                                         uint8_t pos) {
    uint8_t mask = 1 << pos;
    return (var & ~mask) | (val << pos);
}

uint8_t Adafruit_AS7341::checkRegisterBit(uint16_t reg, uint8_t pos) {
    return (readRegisterByte(reg) >> pos) & 0x01;
}

bool Adafruit_AS7341::modifyRegisterBit(uint16_t reg,
                                        bool val,
                                        uint8_t pos) {
    uint8_t r = readRegisterByte(reg);
    r = modifyBitInByte(r, val, pos);
    return writeRegisterByte(reg, r);
}

bool Adafruit_AS7341::modifyRegisterMultipleBit(uint16_t reg,
                                                uint8_t value,
                                                uint8_t pos,
                                                uint8_t bits) {
    uint8_t r = readRegisterByte(reg);
    uint8_t mask = ((1 << bits) - 1) << pos;
    r = (r & ~mask) | ((value << pos) & mask);
    return writeRegisterByte(reg, r);
}







bool Adafruit_AS7341::getIsDataReady()
{
    return checkRegisterBit(AS7341_STATUS2, 6);
}

void Adafruit_AS7341::setSMUXLowChannels(bool f1_f4)
{
    enableSpectralMeasurement(false);
    setSMUXCommand(AS7341_SMUX_CMD_WRITE);

    if (f1_f4) {
        setup_F1F4_Clear_NIR();
    } else {
        setup_F5F8_Clear_NIR();
    }

    enableSMUX();
}


bool Adafruit_AS7341::setSMUXCommand(as7341_smux_cmd_t command)
{
    // CFG6 bits [4:3]
    return modifyRegisterMultipleBit(AS7341_CFG6,
                                     (uint8_t)command,
                                     3,
                                     2);
}

/* ================= LED CONTROL ================= */

bool Adafruit_AS7341::enableLED(bool en)
{
    // LED enable bit = bit 0 of AS7341_LED (0x74)
    uint8_t reg = readRegisterByte(AS7341_LED);
    reg = modifyBitInByte(reg, en, 0);
    return writeRegisterByte(AS7341_LED, reg);
}

bool Adafruit_AS7341::setLEDCurrent(uint16_t ma)
{
    /*
     * AS7341 LED current:
     * Bits [7:1] = current setting
     * Bit [0]    = LED enable
     *
     * Datasheet: ILED = 2 mA * value
     * Max allowed = 258 mA (127 * 2)
     */

    if (ma > 258)
        ma = 258;

    uint8_t current = ma / 2;  // convert mA to register value

    uint8_t reg = readRegisterByte(AS7341_LED);
    reg &= 0x01;               // keep enable bit
    reg |= (current << 1);     // set current bits

    return writeRegisterByte(AS7341_LED, reg);
}



void Adafruit_AS7341::setup_F1F4_Clear_NIR(void)
{
    writeRegisterByte(0x00, 0x30);
    writeRegisterByte(0x01, 0x01);
    writeRegisterByte(0x02, 0x00);
    writeRegisterByte(0x03, 0x00);
    writeRegisterByte(0x04, 0x00);
    writeRegisterByte(0x05, 0x42);
    writeRegisterByte(0x06, 0x00);
    writeRegisterByte(0x07, 0x00);
    writeRegisterByte(0x08, 0x50);
    writeRegisterByte(0x09, 0x00);
    writeRegisterByte(0x0A, 0x00);
    writeRegisterByte(0x0B, 0x00);
    writeRegisterByte(0x0C, 0x20);
    writeRegisterByte(0x0D, 0x04);
    writeRegisterByte(0x0E, 0x00);
    writeRegisterByte(0x0F, 0x30);
    writeRegisterByte(0x10, 0x01);
    writeRegisterByte(0x11, 0x50);
    writeRegisterByte(0x12, 0x00);
    writeRegisterByte(0x13, 0x06);
}



void Adafruit_AS7341::setup_F5F8_Clear_NIR(void)
{
    writeRegisterByte(0x00, 0x00);
    writeRegisterByte(0x01, 0x00);
    writeRegisterByte(0x02, 0x00);
    writeRegisterByte(0x03, 0x40);
    writeRegisterByte(0x04, 0x02);
    writeRegisterByte(0x05, 0x00);
    writeRegisterByte(0x06, 0x10);
    writeRegisterByte(0x07, 0x03);
    writeRegisterByte(0x08, 0x50);
    writeRegisterByte(0x09, 0x10);
    writeRegisterByte(0x0A, 0x03);
    writeRegisterByte(0x0B, 0x00);
    writeRegisterByte(0x0C, 0x00);
    writeRegisterByte(0x0D, 0x00);
    writeRegisterByte(0x0E, 0x24);
    writeRegisterByte(0x0F, 0x00);
    writeRegisterByte(0x10, 0x00);
    writeRegisterByte(0x11, 0x50);
    writeRegisterByte(0x12, 0x00);
    writeRegisterByte(0x13, 0x06);
}






