#ifndef __RGB_H__
#define __RGB_H__

#include "sys.h"

#define RGB_RED_PIN    GPIO_Pin_9
#define RGB_GREEN_PIN  GPIO_Pin_8
#define RGB_BLUE_PIN   GPIO_Pin_7
#define RGB_PORT       GPIOC

// RGB颜色枚举
typedef enum {
    COLOR_BLACK = 0,   // 关
    COLOR_RED,         // 红
    COLOR_GREEN,       // 绿
    COLOR_BLUE,        // 蓝
} RGB_Color;

void RGB_Init(void);

void RGB_SetColor(RGB_Color color);



#endif

