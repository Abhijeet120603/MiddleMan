/*
 * ds18b20.h
 *
 *  Created on: Aug 18, 2026
 *      Author: HP
 */

#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "main.h"
#include <stdint.h>

// Function prototypes
int DS18B20_Start(void);
void DS18B20_Write(uint8_t data);
uint8_t DS18B20_Read(void);
float DS18B20_GetTemperature(void);
int DS18B20_CheckPresence(void);

// Global variables for temperature reading
extern int16_t Temp;
extern float Temperature;
extern uint8_t temp_sens_pres;     // 0: sensor not found, 1: sensor found
extern uint8_t temp_out_of_range;  // 1: temp > 40°C, 0: temp <= 40°C

#endif /* INC_DS18B20_H_ */
