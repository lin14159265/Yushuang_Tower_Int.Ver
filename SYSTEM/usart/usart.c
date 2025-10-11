/**
 ******************************************************************************
 * @ 名称  串口重定向printf程序
 * @ 版本  STD 库 V3.5.0
 * @ 描述  本文件的程序用于将某个串口重定向为printf方法
 * 			修改 USART_PORT，可以实现不同 USART 的 printf 方法
 * @ 注意  本程序仅用于学习使用，适合嵌入式虚拟仿真实验教学平台及其配套硬件
 *	       受服务器性能和网络影响，仿真平台上程序的运行速度会比真实设备慢一些
 ******************************************************************************
 */
#include "sys.h"
#include "usart.h"
#include "delay.h"
#include <string.h>
// 想重定向哪个 USAPT 就修改下面这个宏
#define USART_PORT   USART2
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
// 确保程序中不包含任何半主机的函数
#pragma import(__use_no_semihosting)
// 标准库需要的支持函数
struct __FILE
{
int handle;
};
FILE __stdout;
// 定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
x = x;
}
// 重定义fputc函数
// int fputc(int ch, FILE *f)
// {
// 	while ((USART_PORT->SR & 0X40) == 0)
// 		; // 循环发送,直到发送完毕
// 	USART_PORT->DR = (u8)ch;
// 	return ch;
// }
int _write(int fd, char *pBuffer, int size)
{
int i = 0;
for (i = 0; i < size; i++)
{
while (USART_GetFlagStatus(USART_PORT, USART_FLAG_TXE) == RESET)
;
USART_SendData(USART_PORT, (uint8_t)*pBuffer++);
}
return size;
}