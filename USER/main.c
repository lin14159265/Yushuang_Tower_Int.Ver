#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "delay.h"
#include "stm32f10x_gpio.h"
#include "usart.h"
#include "sys.h"
#include <stdio.h>
#include "stdlib.h"
#include "time.h"
#include "UART_DISPLAY.h"
#include "Frost_Detection.h"
#include "simulation_model.h"
#include "UART_SENSOR.h"
#include "ds18b20.h"
#include "dht11.h"
#include "fan.h"
#include "led.h"
#include "beep.h"
#include "key.h"
#include "Relay.h"
#include "tft.h"
#include "tft_driver.h"
#include "onenet_mqtt.h"


// 作物霜冻临界温度（可根据作物类型调整）
float Crop_Critical_Temp;

float get_critical_temp(int crop_stage) 
{
    switch(crop_stage) 
    {
        case STAGE_TIGHT_CLUSTER: return -3.9f;
        case STAGE_FULL_BLOOM:    return -2.9f;
        case STAGE_SMALL_FRUIT:   return -2.3f; 
        default:                  return 1.0f; // 默认安全值
    }
}

void read_all_environmental_data(EnvironmentalData_t* data);
void System_CloseAll(void);

uint8_t high; //模拟高度变量
uint8_t DATA_Flag = 0;//判断数据是否准备好，模拟高度的一环
uint8_t en_count;//模拟实现时间计数器
uint8_t en_count_flag;//模拟时间计数器标志位
uint8_t CloseAll_flag;
uint8_t g_simulation_tick_flag = 0;


// 全局环境数据
EnvironmentalData_t env_data;
InversionLayerInfo_t current_inversion;
InterventionMethod_t Intervention_Method;
SystemCapabilities_t SysAbilities = {1,1,1};
InterventionPowers_t powers = {0,0,0};
SystemStatus_t system_status;

/****** 风速传感器操作变量 ******/
float	 wind_speed = 0.0;		    // 风速值
uint16_t wind_power = 0;	        // 风力等级
char	 air_receved_flag = FREE;   // 接收状态标志
uint16_t air_error_count = 0;	    // 环境参数传感器异常次数 当传感器连续异常多次，即认为该传感器有问题
uint16_t busy_count = 0;	        // 用于记录等待传感器数据的时间

/****** 温湿度传感器操作变量 ******/
float humidity;                      //湿度值
int humidity_temp;
float temperature;                   //环境温度值
int temperature_temp;     

int mqtt_flag=0;

int main()
{
    // 调试串口初始化           使用 USART1、波特率 115200
    USART1_Init(115200);
    USART2_Init(115200);
    
    // ModBUS传感器初始化	    使用 UART3、波特率 9600
    ModBUS_Init();
    //ds18b20传感器初始化
    DS18B20_InitAll();
    //dht11传感器初始化
    while (DHT11_Init())
    {
        delay_ms(100);
    }
    //模拟风扇初始化
    Servo_Init();
    Fan_TIM2_PWM_Init();
    //模拟警报系统初始化
    Buzzer_Init();
    LED_Init();
    //按键初始化
    EXTI_KEY_Init();
    //继电器初始化
    Relay_Init();
    //模拟水泵和加热器初始化
    WaterPump_And_Heater_Init();
    //模拟环境初始化
    TIM4_MainTick_Init(300);
    srand(time(NULL));
    System_CloseAll();
    

    Robust_Initialize_And_Connect_MQTT();
    MQTT_Subscribe_All_Topics();
    
    //屏幕初始化
    Lcd_Init();
    Lcd_Clear(GRAY0); 
    
    // -- 绘制标题区 --
    Gui_DrawFont_GBK16(5, 2, BLUE, GRAY0, "御霜塔-霜冻预警"); 
    //Gui_DrawFillRect(0, 0, 139, 20, BLUE);  // 标题蓝底
    //Gui_DrawFont_GBK16(5, 2, WHITE, BLUE, "御霜塔-霜冻预警");  // 白字蓝底
    Gui_DrawLine(0, 20, 139, 20, GRAY1);  // 分隔线
    
    // 绘制核心数据区 
    
    Gui_DrawFont_GBK16(5-4, 22, BLACK, GRAY0, "T1:");
    Gui_DrawFont_GBK16(69-4, 22, BLACK, GRAY0, "T2:");   
    Gui_DrawFont_GBK16(5-4, 42, BLACK, GRAY0, "T3:");  
    Gui_DrawFont_GBK16(69-4, 42, BLACK, GRAY0, "T4:");
    Gui_DrawFont_GBK16(0, 62, BLACK, GRAY0, " Amb Temp:");
    Gui_DrawFont_GBK16(5, 82, BLACK, GRAY0, "风速:");
    Gui_DrawFont_GBK16(5, 103, BLACK, GRAY0, "湿度:");
    

    while (1)
    {

        Handle_Serial_Reception();
        //感知层
        Crop_Critical_Temp = get_critical_temp(STAGE_MATURATION);
        read_all_environmental_data(&env_data);

        if(DATA_Flag == 1)
        {
            mqtt_flag=1;

            //决策层
            // 2. 分析逆温层
            current_inversion = Analyze_Inversion_Layer(&env_data);

             // 3. 判断什么干预方法
            Intervention_Method = Determine_Optimal_Intervention(&current_inversion, &SysAbilities, &env_data, Crop_Critical_Temp);

            // --- C. 控制量计算层 (Control Calculation) ---

            switch (Intervention_Method) 
            {
                case INTERVENTION_NONE:
                {
                    LED_SetColor(COLOR_GREEN);
                    Water_Pump_OFF();
                    Heater_OFF();
                    Fan_OFF();
                    Set_Servo_Angle(0);
    
                    break;
                }
                case INTERVENTION_SPRINKLERS:
                {
                    printf("INTERVENTION_SPRINKLERS\r\n");
                    Heater_OFF();
                    Fan_OFF();
                    Set_Servo_Angle(0);                   
                    LED_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Water_Pump_ON();
                    powers.sprinkler_power = calculate_sprinkler_power(&env_data,Crop_Critical_Temp);
                    Sprinkler_Set_Power(powers.sprinkler_power);
                    break;
                }
                case INTERVENTION_FANS_ONLY:
                {
                    printf("INTERVENTION_FANS_ONLY\r\n");
                    Heater_OFF();
                    Water_Pump_OFF();                 
                    LED_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Fan_ON();
                    float target_height = calculate_optimal_intervention_height(&current_inversion);
                    float Angle = calculate_servo_angle(target_height);
                    Set_Servo_Angle((uint16_t)Angle);
                    powers.fan_power = calculate_fan_power(&current_inversion,wind_speed);
                    Fan_Set_Speed(powers.fan_power);
                    break;
                }
                case INTERVENTION_HEATERS_ONLY:
                {
                    printf("INTERVENTION_HEATERS_ONLY\r\n");
                    Fan_OFF();
                    Water_Pump_OFF();
                    Set_Servo_Angle(0);
                    LED_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Heater_ON();
                    powers.heater_power = calculate_heater_power(&env_data,Crop_Critical_Temp);
                    Heater_Set_Power(powers.heater_power);
                    break;
                }
                case INTERVENTION_FANS_THEN_HEATERS:
                {
                    printf("INTERVENTION_FANS_THEN_HEATERS\r\n");
                    Water_Pump_OFF();
                    LED_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Heater_ON();
                    Fan_ON();
                    float target_height = calculate_optimal_intervention_height(&current_inversion);
                    float Angle = calculate_servo_angle(target_height);
                    Set_Servo_Angle((uint16_t)Angle);
                    powers.fan_power = calculate_fan_power(&current_inversion,wind_speed);
                    Fan_Set_Speed(powers.fan_power);
                    powers.heater_power = calculate_heater_power(&env_data,Crop_Critical_Temp);
                    Heater_Set_Power(powers.heater_power);
                    break;
                }
         
            }
            
            if(g_simulation_tick_flag == 1)
            {
                en_count_flag = 0;
                Sim_Update_Environment(&env_data , &powers);
                g_simulation_tick_flag = 0;
                en_count_flag=0;
            }
            /*
            printf("temp1:%f C\r\n",env_data.temperatures[0]);
            printf("temp2:%f C\r\n",env_data.temperatures[1]);
            printf("temp3:%f C\r\n",env_data.temperatures[2]);
            printf("temp4:%f C\r\n",env_data.temperatures[3]);
            printf("wind_speed:%f m/s\r\n",env_data.wind_speed);
            printf("humidity:%f \r\n",env_data.humidity);
            printf("ambient_temp:%f C\r\n",env_data.ambient_temp);
            */


            // 2. 调用MQTT上报函数，并传入 system_status 结构体的地址
            if(mqtt_flag==1)
            {

                system_status.env_data = &env_data;
                system_status.capabilities = &SysAbilities;
                system_status.method = Intervention_Method;
                system_status.Powers = &powers;
                // 当前作物阶段是硬编码的，后续可以改为可配置的全局变量
                system_status.crop_stage = STAGE_MATURATION; 
                MQTT_Publish_All_Data_Adapt(&system_status);
                Display_All_Data(&env_data);
                mqtt_flag=0;
            }
            
        }
          
    }
}

void read_all_environmental_data(EnvironmentalData_t* data)
{
    if(high < 4)
    {
        data->temperatures[high] = DS18B20_Get_Temp(high);
    }
    DHT11_Read_Data(&temperature_temp,&humidity_temp);
    data->humidity = humidity_temp / 10.0f;
    if(high == 1)//当高度为两米的时候读
    {
        data->ambient_temp = temperature_temp / 10.0f;
    }
    

    Get_Wind_Data(&wind_speed,&wind_power);
    data->wind_speed = wind_speed;
    //data->pressure = 1013.0;

}

void System_CloseAll(void)
{
    LED_SetColor(COLOR_BLACK);
    Water_Pump_OFF();
    Heater_OFF();
    Fan_OFF();
    Set_Servo_Angle(0);
    
}

void EXTI9_5_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
        
        high++;
        if(high == 5)
        {
            high = 0;
        }


        EXTI_ClearITPendingBit(EXTI_Line8); // 清除中断标志
    }
    
    if(EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        DATA_Flag = 1;
        EXTI_ClearITPendingBit(EXTI_Line9); // 清除中断标志
    }
}

// EXTI15_10中断服务函数（处理PC13）
void EXTI15_10_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        DATA_Flag = 0;
        System_CloseAll();
        EXTI_ClearITPendingBit(EXTI_Line13); // 清除中断标志
    }
}

// TIM4中断服务函数
void TIM4_IRQHandler(void) 
{
    // 检查是否是TIM4的更新中断
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) 
    {
        if(en_count_flag == 0)
        {
            g_simulation_tick_flag = 1;
            en_count++;
            en_count_flag = 1;
        }
        // 清除中断标志位，否则会不停地进入中断
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}

