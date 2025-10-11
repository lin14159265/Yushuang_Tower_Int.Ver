#ifndef __LED_H__
#define __LED_H__

#include "sys.h"

#define LED_RED_PIN    GPIO_Pin_13
#define LED_GREEN_PIN  GPIO_Pin_12
#define LED_BLUE_PIN   GPIO_Pin_14
#define LED_PORT       GPIOA

// LED颜色枚举
typedef enum {
    COLOR_BLACK = 0,   // 关
    COLOR_RED,         // 红
    COLOR_GREEN,       // 绿
    COLOR_BLUE         // 蓝
} LED_Color;

void LED_Init(void);

void LED_SetColor(LED_Color color);



#endif

