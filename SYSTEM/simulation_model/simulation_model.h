#ifndef __SIMULATION_MODEL_H
#define __SIMULATION_MODEL_H

#include "Frost_Detection.h" // 需要 EnvironmentalData_t
#include <stdint.h>



extern uint8_t en_count;//模拟实现时间计数器
// 根据自然降温和干预功率，更新整个环境的状态
void Sim_Update_Environment(EnvironmentalData_t* env_data, InterventionPowers_t* powers);
void TIM4_MainTick_Init(u16 period_ms);
#endif