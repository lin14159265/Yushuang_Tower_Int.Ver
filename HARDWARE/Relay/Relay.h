#ifndef __RELAY_H__
#define __RELAY_H__

#include "sys.h"



void Relay_Init(void);
void WaterPump_And_Heater_Init(void);
void Heater_Set_Power(uint8_t percent);
void Sprinkler_Set_Power(uint8_t percent);
void Water_Pump_ON(void);
void Water_Pump_OFF(void);
void Heater_ON(void);
void Heater_OFF(void);
void Fan_ON(void);
void Fan_OFF(void);

#endif

