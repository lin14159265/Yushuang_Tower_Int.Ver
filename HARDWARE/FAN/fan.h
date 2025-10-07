#ifndef __FAN_H
#define __FAN_H

#include "sys.h" 

// 假设风扇安装在1.0米高度，距离果树水平距离3.0米
#define FAN_INSTALL_HEIGHT 1.0f   // 风扇安装高度（米）
#define HORIZONTAL_DISTANCE 3.0f  // 风扇到果树的水平距离（米）

void Fan_TIM2_PWM_Init(void);
void Fan_Set_Speed(uint8_t speed_percent);

void Servo_Init(void);
void Set_Servo_Angle(uint16_t angle);
float calculate_servo_angle(float target_height);

#endif