#include "main.h"
#include "i2c_slave.h"
extern I2C_HandleTypeDef hi2c1;
#define RxSize 6
uint8_t RxData[RxSize];
int count = 0;


extern void HAL_I2C_ListenCpltCallback (I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}

extern void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	if(TransferDirection == I2C_DIRECTION_TRANSMIT)  // if the master wants to transmit the data
	{
		HAL_I2C_Slave_Sequential_Receive_IT(hi2c, RxData, 6, I2C_FIRST_AND_LAST_FRAME);
	}
	else  // master requesting the data is not supported yet
	{
		Error_Handler();
	}
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	count++;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}

