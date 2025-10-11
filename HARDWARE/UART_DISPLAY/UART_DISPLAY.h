#ifndef  _UART_DISPLAY_H_
#define  _UART_DISPLAY_H_

#include "UART_SENSOR.h"
#include "stm32f10x.h"
#include "sys.h"
#include <stdio.h>
#include <string.h>

/*****************************************************************************
 ** 移植配置
****************************************************************************/
// 数据接收缓冲区大小，可自行修改
#define U1_RX_BUF_SIZE            1024              // 配置USART1接收缓冲区的大小(字节数)
#define U2_RX_BUF_SIZE            1024

/*****************************************************************************
 ** 全局变量 (无要修改)
****************************************************************************/
typedef struct
{
    uint8_t   USART1InitFlag;                       // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  USART1ReceivedNum;                    // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   USART1ReceivedBuffer[U1_RX_BUF_SIZE]; // 接收到数据的缓存
    volatile uint8_t  USART1RxFrameFlag;            //USART1数据帧接收完成标志

    uint8_t   USART2InitFlag;                       // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  USART2ReceivedNum;                    // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   USART2ReceivedBuffer[U2_RX_BUF_SIZE]; // 接收到数据的缓存

}xUSATR_TypeDef;

extern xUSATR_TypeDef  xUSART;                      // 声明为全局变量,方便记录信息、状态


/*****************************************************************************
 ** 声明全局函数 (无需修改)
****************************************************************************/
// USART1
void    USART1_Init (uint32_t baudrate);                      // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t USART1_GetBuffer (uint8_t* buffer, uint8_t* cnt);     // 获取接收到的数据
void    USART1_SendData (uint8_t* buf, uint16_t cnt);          // 通过中断发送数据，适合各种数据
void    USART1_SendString (char* stringTemp);                 // 通过中断发送字符串，适合字符串，长度在4096个长度内的
void    USART1_SendStringForDMA (char* stringTemp) ;          // 通过DMA发送数据，适合一次过发送数据量特别大的字符串，省了占用中断的时间

// USART2
void    USART2_Init (uint32_t baudrate);                      // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t USART2_GetBuffer (uint8_t* buffer, uint8_t* cnt);     // 获取接收到的数据
void    USART2_SendData (uint8_t* buf, uint8_t cnt);          // 通过中断发送数据，适合各种数据
void    USART2_SendString (char* stringTemp);                 // 通过中断发送字符串，适合字符串，长度在256个长度内的

#endif

