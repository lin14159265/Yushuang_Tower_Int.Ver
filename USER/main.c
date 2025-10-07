#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "UART_SENSOR.h"
#include "delay.h"
#include "stm32f10x_gpio.h"
#include "usart.h"
#include "sys.h"
#include <stdio.h>
#include "Frost_Detection.h"
#include "ds18b20.h"
#include "dht11.h"
#include "fan.h"
#include "rgb.h"
#include "beep.h"
#include "key.h"
#include "Relay.h"
#include "onenet_mqtt.h"
#include "bsp_usart.h"


// 作物霜冻临界温度（可根据作物类型调整）
float Crop_Critical_Temp;
typedef enum {
    STAGE_TIGHT_CLUSTER,             // 紧簇期（花蕾紧密聚集阶段）
    STAGE_FULL_BLOOM,                // 盛花期（花朵完全开放阶段）
    STAGE_SMALL_FRUIT,               // 小果期（果实初形成阶段）
    STAGE_MATURATION                 // 成熟期
} CROP_CRITICAL_TEMPERATURE;

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

uint8_t high; //模拟高度变量
uint8_t DATA_Flag = 0;//判断数据是否准备好，模拟高度的一环

// 全局环境数据
EnvironmentalData_t env_data;
InversionLayerInfo_t current_inversion;
InterventionMethod_t Intervention_Method;
SystemCapabilities_t SysAbilities = {1,1,1};

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
int current_crop_stage=STAGE_TIGHT_CLUSTER;
uint8_t fan_speed = 0;

int main()
{
    // 调试串口初始化           使用 USART1、波特率 115200
    USART1_Init(115200);
    // ModBUS传感器初始化	    使用 UART3、波特率 9600
    ModBUS_Init();
    //ds18b20传感器初始化
    DS18B20_InitAll();
    //dht11传感器初始化
    while (DHT11_Init())
    {
        printf("DHT11 is Error\n");
        delay_ms(100);
    }
    //模拟风扇初始化
    Servo_Init();
    Fan_TIM2_PWM_Init();
    //模拟警报系统初始化
    Buzzer_Init();
    RGB_Init();
    //按键初始化
    EXTI_KEY_Init();
    //继电器初始化
    Relay_Init();
    //MQTT初始化
    Robust_Initialize_And_Connect_MQTT();

    // 【修正点 1】: 在 main 函数开头定义用于打包的结构体变量
    SystemStatus_t current_system_status;
    
   
    
    while (1)
    {
        read_all_environmental_data(&env_data);
        Crop_Critical_Temp = get_critical_temp(STAGE_MATURATION);

       
        if(DATA_Flag == 1)
        {

            
            // 2. 分析逆温层
            current_inversion = Analyze_Inversion_Layer(&env_data);

             // 3. 判断什么干预方法
            Intervention_Method = Determine_Optimal_Intervention(&current_inversion, &SysAbilities, &env_data, Crop_Critical_Temp);
            
            switch (Intervention_Method) 
            {
                case INTERVENTION_NONE:
                {
                    RGB_SetColor(COLOR_GREEN);
                    break;
                }
                case INTERVENTION_SPRINKLERS:
                {
                    printf("INTERVENTION_SPRINKLERS\r\n");
                    RGB_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Water_Pump_ON();
                    break;
                }
                case INTERVENTION_FANS_ONLY:
                {
                    printf("INTERVENTION_FANS_ONLY\r\n");
                    RGB_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Fan_ON();
                    float target_height = calculate_optimal_intervention_height(&current_inversion);
                    float Angle = calculate_servo_angle(target_height);
                    Set_Servo_Angle((uint16_t)Angle);
                    uint8_t fan_speed = calculate_fan_power(&current_inversion,wind_speed);
                    Fan_Set_Speed(fan_speed);
                    break;
                }
                case INTERVENTION_HEATERS_ONLY:
                {
                    printf("INTERVENTION_HEATERS_ONLY\r\n");
                    RGB_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Heater_ON();
                    break;
                }
                case INTERVENTION_FANS_THEN_HEATERS:
                {
                    printf("INTERVENTION_FANS_THEN_HEATERS\r\n");
                    RGB_SetColor(COLOR_RED);
                    Buzzer_Alarm(3);
                    Heater_ON();
                    Fan_ON();
                    float target_height = calculate_optimal_intervention_height(&current_inversion);
                    float Angle = calculate_servo_angle(target_height);
                    Set_Servo_Angle((uint16_t)Angle);
                    uint8_t fan_speed = calculate_fan_power(&current_inversion,wind_speed);
                    Fan_Set_Speed(fan_speed);
                    break;
                }
         
            }
            



            printf("temp1:%f C\r\n",env_data.temperatures[0]);
            printf("temp2:%f C\r\n",env_data.temperatures[1]);
            printf("temp3:%f C\r\n",env_data.temperatures[2]);
            printf("temp4:%f C\r\n",env_data.temperatures[3]);
            printf("wind_speed:%f m/s",env_data.wind_speed);
            printf("humidity:%f \r\n",env_data.humidity);
            printf("ambient_temp:%f C\r\n",env_data.ambient_temp);

            // 1. 填充 SystemStatus_t 结构体
            current_system_status.env_data = &env_data;
            current_system_status.capabilities = &SysAbilities;
            current_system_status.method = Intervention_Method;
            current_system_status.fan_power = fan_speed;
            current_system_status.crop_stage = current_crop_stage;


            MQTT_Publish_All_Data_Adapt(&current_system_status);
        }
        

    }

}

void read_all_environmental_data(EnvironmentalData_t* data)
{
    data->temperatures[high] = DS18B20_Get_Temp(high);

    DHT11_Read_Data(&temperature_temp,&humidity_temp);
    data->humidity = humidity_temp / 10.0f;
    if(high == 1)//当高度为两米的时候读
    {
        data->ambient_temp = temperature_temp / 10.0f;
    }
    

    Get_Wind_Data(&wind_speed,&wind_power);
    data->wind_speed = wind_speed;
    data->pressure = 1013.0;

}

void System_CloseAll(void)
{
    RGB_SetColor(COLOR_BLACK);
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
        if(high == 4)
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

