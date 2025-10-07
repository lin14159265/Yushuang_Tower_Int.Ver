#include "rgb.h"


void RGB_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // 配置RGB引脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = RGB_RED_PIN | RGB_GREEN_PIN | RGB_BLUE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RGB_PORT, &GPIO_InitStructure);
    
    // 初始状态关闭所有LED
    GPIO_SetBits(RGB_PORT, RGB_RED_PIN | RGB_GREEN_PIN | RGB_BLUE_PIN);
}


// 设置RGB颜色
void RGB_SetColor(RGB_Color color)
{
    // 先关闭所有LED
    GPIO_SetBits(RGB_PORT, RGB_RED_PIN | RGB_GREEN_PIN | RGB_BLUE_PIN);
    
    // 根据颜色设置相应的LED
    switch(color)
    {
        case COLOR_RED:
            GPIO_ResetBits(RGB_PORT, GPIO_Pin_9);
            break;
        case COLOR_GREEN:
            GPIO_ResetBits(RGB_PORT, GPIO_Pin_8);
            break;
        case COLOR_BLUE:
            GPIO_ResetBits(RGB_PORT, GPIO_Pin_7);
            break;
        case COLOR_BLACK:
        default:
            // 保持所有LED关闭
            break;
    }
}