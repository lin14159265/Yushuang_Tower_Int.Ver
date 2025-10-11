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
void WaterPump_And_Heater_Init()
{
        GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    //配置 PA8 和 PA11 为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_11; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //初始化TIM1
    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0; // 初始占空比都为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    // 配置通道1 (PA8 - 加热器)
    TIM_OC1Init(TIM1, &TIM_OCInitStructure); 
    // 配置通道4 (PA11 - 洒水器)
    TIM_OC4Init(TIM1, &TIM_OCInitStructure); 

    
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable); 

    
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_ARRPreloadConfig(TIM1, ENABLE); 

    
    TIM_Cmd(TIM1, ENABLE);

}

void Heater_Set_Power(uint8_t percent)
{
    if (percent > 100) percent = 100;
    
    uint16_t ccr_value = (uint16_t)(percent * 10); 
    TIM_SetCompare1(TIM1, ccr_value); 
}


void Sprinkler_Set_Power(uint8_t percent)
{
    if (percent > 100) percent = 100;
    
    uint16_t ccr_value = (uint16_t)(percent * 10); 
    TIM_SetCompare4(TIM1, ccr_value); 
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


