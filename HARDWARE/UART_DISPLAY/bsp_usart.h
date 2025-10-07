#ifndef __BSP__USART_H
#define __BSP__USART_H

 /***********************************************************************************************************************************
 ** 【文件名称】  bsp_usart.h
 **
 ** 【文件功能】  定义引脚、定义全局结构体、声明全局函数
 **               本文件简化的USART的初始化，及完善了收、发功能函数;初始化后调用函数即可使用;
 **
 ** 【适用平台】  STM32F103 + 标准库v3.5 + keil5
 **
 ** 【硬件重点】  调用USART1_Init()函数时，将按以下默认引脚进行初始化：
 **               1- USART1 PA9 (TX), PA10 (RX)
 **
 ** 【代码说明】  本文件的收发机制, 经多次修改, 已比较完善.
 **               初始化: 只需调用：USARTx_Init(波特率), 函数内已做好引脚及时钟配置;
 **               发 送 : 方法1_发送任意长度字符串: USARTx_SendString (char* stringTemp);
 **                       方法2_发送指定长度数据  : USARTx_SendData (uint8_t* buf, uint8_t cnt);
 **               接 收 : 方式1_通过全局函数: USARTx_GetBuffer (uint8_t* buffer, uint8_t* cnt);　// 当有数据时，返回1; 本函数已清晰地示例了接收机制;
 **                       方式2_通过判断xUSART.USART1ReceivedNum>0;
 **                              如在while中不断轮询，或在任何一个需要的地方判断这个接收字节长度变量值．示例：
 **                              while(1){
 **                                  if(xUSART.USART1ReceivedNum > 0)
 **                                  {
 **                                      printf((char*)xUSART.USART1ReceivedBuffer);          // 示例1: 如何输出成字符串
 **                                      uint16_t GPS_Value = xUSART.USART1ReceivedBuffer[1]; // 示例2: 如何读写其中某个成员的数据
 **                                      xUSART.USART1ReceivedNum = 0;                     // 重要：处理完数据后, 必须把接收到的字节数量值清0
 **                                  }
 **                              }
 ************************************************************************************************************************************/
#include <stm32f10x.h>
#include <stdio.h>
#include "string.h"       // C标准库头文件: 字符数组常用：strcpy()、strncpy()、strcmp()、strlen()、strnset()


/*****************************************************************************
 ** 移植配置
****************************************************************************/
// 数据接收缓冲区大小，可自行修改
#define U1_RX_BUF_SIZE            1024              // 配置USART1接收缓冲区的大小(字节数)


/*****************************************************************************
 ** 全局变量 (无要修改)
****************************************************************************/
typedef struct
{
    uint8_t   USART1InitFlag;                       // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  USART1ReceivedNum;                    // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   USART1ReceivedBuffer[U1_RX_BUF_SIZE]; // 接收到数据的缓存

}xUSATR_TypeDef;

extern xUSATR_TypeDef  xUSART;                      // 声明为全局变量,方便记录信息、状态


/*****************************************************************************
 ** 声明全局函数 (无需修改)
****************************************************************************/
// USART1
void    USART1_Init (uint32_t baudrate);                      // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t USART1_GetBuffer (uint8_t* buffer, uint8_t* cnt);     // 获取接收到的数据
void    USART1_SendData (uint8_t* buf, uint8_t cnt);          // 通过中断发送数据，适合各种数据
void    USART1_SendString (char* stringTemp);                 // 通过中断发送字符串，适合字符串，长度在4096个长度内的
void    USART1_SendStringForDMA (char* stringTemp) ;          // 通过DMA发送数据，适合一次过发送数据量特别大的字符串，省了占用中断的时间


#endif
