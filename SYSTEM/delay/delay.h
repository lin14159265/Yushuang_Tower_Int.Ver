/**
 ******************************************************************************
 * @ 名称  延时程序
 * @ 版本  STD 库 V3.5.0
 * @ 描述  本文件的程序用于STM32精确延时
 * @ 注意  本程序仅用于学习使用，适合嵌入式虚拟仿真实验教学平台及其配套硬件
 *	       受服务器性能和网络影响，仿真平台上程序的运行速度会比真实设备慢一些
 ******************************************************************************
 */
#ifndef __DELAY_H
#define __DELAY_H 			   
#include "sys.h"
#include <stdint.h>  // 引入标准整型  





void delay(uint32_t nus);
void delay_ms(uint32_t nms);
void delay_us(uint32_t nus);

void System_SysTickInit(void);
uint64_t System_GetTimeMs(void);
#endif





























