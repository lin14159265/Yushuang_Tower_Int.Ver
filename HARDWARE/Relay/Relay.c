#include "Relay.h"
#include "stm32f10x.h"

void Relay_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_1;				 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_15);	
    GPIO_SetBits(GPIOB,GPIO_Pin_14);
    GPIO_SetBits(GPIOB,GPIO_Pin_1);	

}

void Water_Pump_ON(void)
{

    GPIO_ResetBits(GPIOB,GPIO_Pin_1);

}

void Water_Pump_OFF(void)
{

    GPIO_SetBits(GPIOB,GPIO_Pin_1);	

}

void Heater_ON(void)
{

    GPIO_ResetBits(GPIOB,GPIO_Pin_15);

}

void Heater_OFF(void)
{

    GPIO_SetBits(GPIOB,GPIO_Pin_15);

}

void Fan_ON(void)
{

    GPIO_ResetBits(GPIOB,GPIO_Pin_14);

}

void Fan_OFF(void)
{

    GPIO_SetBits(GPIOB,GPIO_Pin_14);

}

