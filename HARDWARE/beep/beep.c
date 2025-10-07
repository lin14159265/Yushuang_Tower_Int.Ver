#include "beep.h"
#include "delay.h"


void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOC时钟
    
    
    // 配置PC13为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 初始状态关闭蜂鸣器
    GPIO_ResetBits(GPIOC, GPIO_Pin_6);
}

// 蜂鸣器开启
void Buzzer_On(void)
{
    GPIO_SetBits(GPIOC, GPIO_Pin_6);
}

// 蜂鸣器关闭
void Buzzer_Off(void)
{
    GPIO_ResetBits(GPIOC, GPIO_Pin_6);
}

void Buzzer_Alarm(uint8_t times)
{
    for(uint8_t i = 0; i < times; i++)
    {
        Buzzer_On();
        delay_ms(150);
        Buzzer_Off();
        delay_ms(150);
    }
}

