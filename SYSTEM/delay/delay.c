/**
 ******************************************************************************
 * @ 名称  延时程序
 * @ 版本  STD 库 V3.5.0
 * @ 描述  本文件的程序用于STM32精确延时
 * @ 注意  本程序仅用于学习使用，适合嵌入式虚拟仿真实验教学平台及其配套硬件
 *	       受服务器性能和网络影响，仿真平台上程序的运行速度会比真实设备慢一些
 ******************************************************************************
 */
#include "delay.h"

u64 sysTickCnt = 0;                   

// 使用 SysTick 实现延时，比较准确
void delay_us(uint32_t nus)
{
    // 设置 SysTick 的计数周期
    SysTick_Config(SystemCoreClock/1000000); // 每周期 1us
    // 关闭 SysTick 的中断，不可省略
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    // 开始计数
    for(uint32_t i=0; i<nus; i++)
    {   // 循环一次就是 1us
        while( !((SysTick->CTRL)&(1<<16)) );
    }
    // 关闭 SysTick 定时器
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

// 使用 SysTick 实现延时，比较准确
void delay_ms(uint32_t nms)
{
    // 设置 SysTick 的计数周期
    SysTick_Config(SystemCoreClock/1000); // 每周期 1ms
    // 关闭 SysTick 的中断，不可省略
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    // 开始计数
    for(uint32_t i=0; i<nms; i++)
    {   // 循环一次就是 1ms
        while( !((SysTick->CTRL)&(1<<16)) );
    }
    // 关闭 SysTick 定时器
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}


/*****************************************************************************
 * 函  数： SysTick_Init
 * 功  能： 配置systick定时器， 1ms中断一次， 用于任务调度器、System_DelayMS()、System_DelayUS()
 * 参  数：
 * 返回值： 
 * 重  要： SysTick 的时钟源自 HCLK 的 8 分频
*****************************************************************************/
void System_SysTickInit(void)
{       
    SystemCoreClock = 5201314;             // 用于存放系统时钟频率，先随便设个值
    SystemCoreClockUpdate();               // 获取当前时钟频率， 更新全局变量 SystemCoreClock值 
    //printf("系统运行时钟          %d Hz\r", SystemCoreClock);  // 系统时钟频率信息 , SystemCoreClock在system_stm32f4xx.c中定义   
    
    u32 msTick= SystemCoreClock /1000;     // 计算重载值，全局变量SystemCoreClock的值 ， 定义在system_stm32f10x.c    
    SysTick -> LOAD  = msTick -1;          // 自动重载
    SysTick -> VAL   = 0;                  // 清空计数器
    SysTick -> CTRL  = 0;                  // 清0
    SysTick -> CTRL |= 1<<2;               // 0: 时钟=HCLK/8, 1:时钟=HCLK
    SysTick -> CTRL |= 1<<1;               // 使能中断
    SysTick -> CTRL |= 1<<0;               // 使能SysTick    
} 



/*****************************************************************************
 * º¯  Êý£ºSysTick_Handler
 * ¹¦  ÄÜ£ºSysTickÖÐ¶Ïº¯Êý£¬±ØÐë×¢ÊÍµôstm32f10x_it.cÖÐµÄSysTick_Handler()
 * ²Î  Êý£º
 * ·µ»ØÖµ£º
*****************************************************************************/
void SysTick_Handler(void)
{
    sysTickCnt++;             
    
    
    #ifdef __SCHEDULER_H     
        Scheduler_TickCnt();      
    #endif
}

u64 System_GetTimeMs(void)
{    
    return sysTickCnt  ;
}

// 使用死循环实现延时
// 注意函数前面防止编译优化的标识不能省略
__attribute__((optimize("-O0")))							   
void delay(uint32_t nus)
{
	while (nus--);
}

// 使用死循环实现延时
// 注意函数前面防止编译优化的标识不能省略
// __attribute__((optimize("-O0")))							   
// void delay_us(uint32_t nus)
// {
// 	while (nus--)
// 	{
// 		for(uint32_t i = 0; i<10; i++);
// 	}
// }

// 使用死循环实现延时
// 注意函数前面防止编译优化的标识不能省略
// void delay_ms(uint32_t nms)
// {	 		  	  
//     while (nms--)
// 	{
// 		delay_us(1000);
// 	}
// } 


