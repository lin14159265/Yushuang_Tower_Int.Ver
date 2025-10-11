#include "led.h"


void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置RGB引脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStructure);
    
    // 初始状态关闭所有LED
    GPIO_ResetBits(LED_PORT, LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN);
}


// 设置LED颜色
void LED_SetColor(LED_Color color)
{
    // 先关闭所有LED
    GPIO_ResetBits(LED_PORT, LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN);
    
    // 根据颜色设置相应的LED
    switch(color)
    {
        case COLOR_RED:
            GPIO_SetBits(LED_PORT, GPIO_Pin_13);
            break;
        case COLOR_GREEN:
            GPIO_SetBits(LED_PORT, GPIO_Pin_12);
            break;
        case COLOR_BLUE:
            GPIO_SetBits(LED_PORT, GPIO_Pin_14);
            break;
        case COLOR_BLACK:
        default:
            break;
    }
}