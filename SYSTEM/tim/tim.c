/**
 ******************************************************************************
 * @ 名称  系统程序
 * @ 版本  STD 库 V3.5.0
 * @ 描述  本文件的程序用于初始化定时器，提供定时器中断或PWM
 * @ 注意  本程序仅用于学习使用，适合嵌入式虚拟仿真实验教学平台及其配套硬件
 *	       受服务器性能和网络影响，仿真平台上程序的运行速度会比真实设备慢一些
 ******************************************************************************
 */
#include "tim.h"

// 初始化 TIM1 中断服务
void TIM1_Init(uint16_t arr, uint16_t psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); //时钟使能

	TIM_TimeBaseStructure.TIM_Period = arr; 	// 计时周期
	TIM_TimeBaseStructure.TIM_Prescaler = psc;  // 分频系数 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;                 // 设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  // TIM向上计数模式
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 8;             // 重复计数器，高级定时器更新计数 8 次才中断一次
	                                                             // 可用于超长时间定时中断
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(      // 使能或者失能指定的TIM中断
		TIM1,          // TIM1
		TIM_IT_Update, // TIM 中断源
		ENABLE         // 使能
		);
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;         // TIM1 中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  // 先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;         // 响应优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM1, ENABLE);  //使能TIMx外设
    
}

void TIM1_UP_IRQHandler(void) 
{   // 检查指定的TIM1中断发生与否
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
	{	/* 在此处添加中断执行内容 */
		
        // 清除TIM1的中断位
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}	     
} 

// 初始化 TIM2 中断服务
void TIM2_Init(uint16_t arr, uint16_t psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //时钟使能

	TIM_TimeBaseStructure.TIM_Period = 10; 
	TIM_TimeBaseStructure.TIM_Prescaler =(7200-1); //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(      // 使能或者失能指定的TIM中断
		TIM2,          // TIM2
		TIM_IT_Update, // TIM 中断源
		ENABLE         // 使能
		);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  // 先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;         // 响应优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM2, ENABLE);  //使能TIMx外设
    
}

void TIM2_IRQHandler(void) 
{	// 检查TIM2中断发生与否 
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{	/* 在此处添加中断执行内容 */

		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);// 清除TIM2的中断位 
	}	     
} 