#include "i2c_slave.h"

extern I2C_HandleTypeDef hi2c1;

#define RX_SIZE 12

uint8_t RxData[RX_SIZE];
volatile uint8_t i2c_cmd_ready = 0;

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c,
                          uint8_t TransferDirection,
                          uint16_t AddrMatchCode)
{
    if (TransferDirection == I2C_DIRECTION_TRANSMIT)
    {
        HAL_I2C_Slave_Sequential_Receive_IT(
            hi2c,
            RxData,
            RX_SIZE,
            I2C_FIRST_AND_LAST_FRAME
        );
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_cmd_ready = 1;   // 🔔 Notify main loop
    HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_EnableListen_IT(hi2c);
}
