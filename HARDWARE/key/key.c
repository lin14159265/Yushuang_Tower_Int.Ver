#include "key.h"
#include "delay.h"

void EXTI_KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure; 
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8 | GPIO_Pin_9;
	  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	  GPIO_Init(GPIOB,&GPIO_InitStructure);  

	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_13 | GPIO_Pin_3 | GPIO_Pin_7;
	  GPIO_Init(GPIOC,&GPIO_InitStructure);

    
    EXTI_InitStructure.EXTI_Line    = EXTI_Line8;
  	EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);
	  EXTI_InitStructure.EXTI_Line    = EXTI_Line9;
	  EXTI_Init(&EXTI_InitStructure);
	  EXTI_InitStructure.EXTI_Line    = EXTI_Line13;
	  EXTI_Init(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line    = EXTI_Line3;
    EXTI_Init(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line    = EXTI_Line7;
    EXTI_Init(&EXTI_InitStructure);
    

    // 配置EXTI9_5中断（处理EXTI5-EXTI9）
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;        
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;    
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;        
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            
    NVIC_Init(&NVIC_InitStructure);

    // 配置EXTI15_10中断（处理EXTI10-EXTI15）
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    // 配置EXTI3中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&NVIC_InitStructure);

  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8 );
	  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9 );
	  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13 );
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3 );
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource7 );
}
