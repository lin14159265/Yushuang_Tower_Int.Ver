#ifndef  _UART_DISPLAY_H_
#define  _UART_DISPLAY_H_

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

// 定义接收缓冲区的最大长度
#define USART1_RX_BUFFER_SIZE   256

// 使用 extern 关键字声明全局变量，以便 main.c 等其他文件可以访问它们
// g_usart1_rx_buffer: 存储从串口接收到的数据
// g_usart1_rx_flag:   接收完成标志 (volatile关键字确保编译器不会优化掉对它的访问)
extern uint8_t g_usart1_rx_buffer[USART1_RX_BUFFER_SIZE];
extern volatile uint8_t g_usart1_rx_flag;

// --- 原有的函数声明 ---
void USART1_Init(uint32_t bound);
void USART1_SendData(uint8_t data);
void USART1_SendString(uint8_t *data, uint16_t len);

// --- 新增的辅助函数声明 ---
void Clear_USART1_Buffer(void);

#endif