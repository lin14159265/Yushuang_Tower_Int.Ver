#ifndef __DS18B20_H
#define __DS18B20_H 
#include "sys.h"   

// 定义DS18B20传感器的数量
#define DS18B20_COUNT 4

// 定义每个传感器使用的独立GPIO端口
#define DS18B20_PORT_0 GPIOA
#define DS18B20_PORT_1 GPIOB
#define DS18B20_PORT_2 GPIOC
#define DS18B20_PORT_3 GPIOD
// 定义每个传感器的独立引脚
#define DS18B20_PIN_0 GPIO_Pin_5
#define DS18B20_PIN_1 GPIO_Pin_0
#define DS18B20_PIN_2 GPIO_Pin_14
#define DS18B20_PIN_3 GPIO_Pin_1

// 用于方便迭代的引脚数组
extern const u16 DS18B20_PINS[DS18B20_COUNT];
extern GPIO_TypeDef * DS18B20_PORT[DS18B20_COUNT];



// 为特定引脚的IO操作函数											   
#define DS18B20_DQ_LOW(GPIO,pin)  GPIO->BRR = pin    // 拉低
#define DS18B20_DQ_HIGH(GPIO,pin) GPIO->BSRR = pin   // 拉高
#define DS18B20_DQ_READ(GPIO,pin) (GPIO->IDR & pin)  // 读取状态


// 函数原型，带 sensor_index 参数
u8 DS18B20_Init(u8 sensor_index);                   // 初始化特定的DS18B20
float DS18B20_Get_Temp(u8 sensor_index);            // 从特定的DS18B20获取温度
void DS18B20_Start(u8 sensor_index);                // 启动特定的DS18B20温度转换
void DS18B20_Write_Byte(u8 sensor_index, u8 dat);   // 向特定的DS18B20写入一个字节
u8 DS18B20_Read_Byte(u8 sensor_index);              // 从特定的DS18B20读取一个字节
u8 DS18B20_Read_Bit(u8 sensor_index);               // 从特定的DS18B20读取一个位
u8 DS18B20_Check(u8 sensor_index);                  // 检测特定的DS18B20是否存在
void DS18B20_Rst(u8 sensor_index);                  // 复位特定的DS18B20    

// 初始化所有DS18B20传感器的函数
void DS18B20_InitAll(void);

#endif















