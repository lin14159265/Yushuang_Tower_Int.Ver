#ifndef __UART_SENSOR_H_
#define __UART_SENSOR_H_

#include "stm32f10x.h"
#include "sys.h" 

extern float	 wind_speed;		// 风速值
extern uint16_t wind_power;	    // 风力等级
extern char	 air_receved_flag;   // 接收状态标志
extern uint16_t air_error_count;	    // 环境参数传感器异常次数 当传感器连续异常多次，即认为该传感器有问题
extern uint16_t busy_count;	        // 用于记录等待传感器数据的时间
// 传感器接收状态的标志值
#define FAIL    -1 	// 失败
#define FREE    0	// 空闲
#define FINISH  1	// 结束
#define BUSY    2	// 工作中
// 允许传感器连续异常的最大值
#define SENSOR_ERROR_THRESHOLD 3
#define LED PBout(1) // PB1

void ModBUS_Init(void);
void USART3_SendString(const unsigned char *data, uint8_t len);
void Execute_ModBUS_Sensor(void);
int Get_Humidity_Value(void);

// 执行空气温湿度传感器数据的读取与显示
void Execute_Sensor_Humidity(void);
// 执行空气二氧化碳传感器数据的读取与显示
void Execute_Sensor_CO2(void);

void Get_Wind_Data(float *speed, uint16_t *power);

#endif
