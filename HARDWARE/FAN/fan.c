#include "stm32f10x.h" 
#include "fan.h"
#include "math.h"
/**************************************************************
功  能：通用定时器2中断初始化
参  数: 无
返回值: 无
**************************************************************/
void Fan_TIM2_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 1. 开启时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
   
    
    // 2. 配置GPIOA1为复用推挽输出 (TIM2_CH2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. 初始化定时器2
    TIM_TimeBaseStructure.TIM_Period = 999;         // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = 71;      // 预分频系数
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 4. 初始化PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;      // PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0;                     // 初始占空比为0
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);               // 初始化通道2
    
    // 5. 使能预装载寄存器
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    
    // 6. 启动定时器
    TIM_Cmd(TIM2, ENABLE);
}

/*******************************************************************
功  能：电动风扇节点模拟
参  数: speed_percent
返回值: 无
********************************************************************/
// 设置风扇速度 (0-100%)
void Fan_Set_Speed(uint8_t speed_percent)
{
    uint16_t compare_value;
    
    // 限制输入范围
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    
    if (speed_percent == 0) {
        // 完全关闭风扇
        TIM_SetCompare2(TIM2, 0);
        return;
    }
    
    // 将百分比转换为PWM比较值
    // 假设ARR设置为1000，那么50%对应500
    compare_value = (speed_percent * TIM2->ARR) / 100;
    
    // 设置占空比
    TIM_SetCompare2(TIM2, compare_value);
}

void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 1. 开启相关时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 2. 配置PA6为复用推挽输出 (TIM3_CH1)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_TimeBaseStructure.TIM_Period = 19999;           // ARR值
    TIM_TimeBaseStructure.TIM_Prescaler = 71;           // 预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // 4. 配置TIM3通道1为PWM模式1
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1500;              // 1.5ms脉宽，中间位置
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // 5. 使能ARR预装载（可选但推荐）
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    // 6. 使能TIM3
    TIM_Cmd(TIM3, ENABLE);
}

void Set_Servo_Angle(uint16_t angle)
{
    // 限制角度范围
    if(angle > 180) angle = 180;
    
    uint16_t pulse = (uint16_t)(500 + angle * (2000.0f / 180.0f));
    TIM_SetCompare1(TIM3, pulse);
}

float calculate_servo_angle(float target_height)
{
    float vertical_distance = target_height - FAN_INSTALL_HEIGHT;
    
    // 使用三角函数计算角度: tan(θ) = 对边/邻边
    // θ = arctan(vertical_distance / HORIZONTAL_DISTANCE)
    float angle_rad = atan2f(vertical_distance, HORIZONTAL_DISTANCE);
    float angle_deg = angle_rad * 180.0f / 3.1415926535f; // 弧度转角度
    
    // 限制角度在安全范围内（0-60度）
    if (angle_deg < 0.0f) angle_deg = 0.0f;
    if (angle_deg > 60.0f) angle_deg = 60.0f;
    
    return angle_deg;

}

