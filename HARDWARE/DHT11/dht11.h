#ifndef __DHT11_H
#define __DHT11_H 
#include "sys.h"   


//IO方向设置
#define DHT11_GPIO_PORT GPIOB
#define DHT11_GPIO_PIN  GPIO_Pin_2
#define DHT11_RCC_CLK   RCC_APB2Periph_GPIOB
////IO操作函数											   
#define	DHT11_DQ_OUT PBout(2) //数据端口	PB12
#define	DHT11_DQ_IN  PBin(2)  //数据端口	PB12

u8 DHT11_Init(void);//初始化DHT11
u8 DHT11_Read_Data(int *temp,int *humi);//读取温湿度
u8 DHT11_Read_Byte(void);//读出一个字节
u8 DHT11_Read_Bit(void);//读出一个位
u8 DHT11_Check(void);//检测是否存在DHT11
void DHT11_Rst(void);//复位DHT11    
#endif




