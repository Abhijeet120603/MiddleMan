//#ifndef INC_I2C_SLAVE_H_
//#define INC_I2C_SLAVE_H_
//#include "main.h"
//void I2C_Slave_Init(void);
//
//#endif



#ifndef INC_I2C_SLAVE_H_
#define INC_I2C_SLAVE_H_

#include "main.h"

extern uint8_t RxData[12];
extern volatile uint8_t i2c_cmd_ready;

#endif
