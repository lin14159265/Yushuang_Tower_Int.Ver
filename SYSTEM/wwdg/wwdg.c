/**
 ******************************************************************************
 * @ 名称  窗口看门狗程序
 * @ 版本  STD 库 V3.5.0
 * @ 描述  本文件的程序用于提供窗口看门狗功能
 * @ 注意  本程序仅用于学习使用，适合嵌入式虚拟仿真实验教学平台及其配套硬件
 *	       受服务器性能和网络影响，仿真平台上程序的运行速度会比真实设备慢一些
 ******************************************************************************
 */
#include "wwdg.h"

//初始化窗口看门狗 	
//tr   :T[6:0],计数器值 
//wr   :W[6:0],窗口值 
//fprer:分频系数（WDGTB）,仅最低2位有效 
//Fwwdg=PCLK1/(4096*2^fprer). 

void WWDG_Init(uint8_t tr,uint8_t wr,uint32_t fprer)
{ 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);  //   WWDG时钟使能

	WWDG_SetPrescaler(fprer);////设置IWDG预分频值

	WWDG_SetWindowValue(wr);//设置窗口值

	WWDG_Enable(tr&WWDG_CNT);	 //使能看门狗 ,	设置 counter .                  

	WWDG_ClearFlag();//清除提前唤醒中断标志位 

	WWDG_NVIC_Init();//初始化窗口看门狗 NVIC

	WWDG_EnableIT(); //开启窗口看门狗中断
} 
//重设置WWDG计数器的值
void WWDG_Set_Counter(uint8_t cnt)
{
    WWDG_Enable(cnt);//使能看门狗 ,	设置 counter .	 
}
//窗口看门狗中断服务程序
void WWDG_NVIC_Init()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = WWDG_IRQn;    //WWDG中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;   //抢占2，子优先级3，组2	
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;	 //抢占2，子优先级3，组2	
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE; 
	NVIC_Init(&NVIC_InitStructure);//NVIC初始化
}

void WWDG_IRQHandler(void)
{
	WWDG_SetCounter(WWDG_CNT);	  //当禁掉此句后,窗口看门狗将产生复位

	WWDG_ClearFlag();	  //清除中断标志位
}
