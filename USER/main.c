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
#include "iwdg.h" 

// ================= [新] 任务调度时间配置 =================
#define SENSOR_READ_INTERVAL_MS 2000      // 定义传感器读取周期为 2000毫秒
#define MQTT_REPORT_INTERVAL_MS 15000     // 定义MQTT数据上报周期为 15000毫秒
#define MQTT_KEEPALIVE_INTERVAL_MS 60000  // 定义MQTT心跳/重连检查周期为 60000毫秒
// =========================================================

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
uint8_t DATA_Flag = 0;//判断数据是否准备好
uint8_t en_count;//模拟实现时间计数器
uint8_t en_count_flag;//模拟时间计数器标志位
uint8_t g_simulation_tick_flag = 0;//模拟时间标志位
static uint8_t g_upload_step = 0;
static DeviceStatus g_status_to_upload;

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
uint16_t air_error_count = 0;	    // 环境参数传感器异常次数
uint16_t busy_count = 0;	        // 用于记录等待传感器数据的时间

/****** 温湿度传感器操作变量 ******/
float humidity;                      //湿度值
int humidity_temp;
float temperature;                   //环境温度值
int temperature_temp;     


int main()
{
    // [新] 2. 初始化SysTick作为系统心跳
    System_SysTickInit(); 

    // 调试串口初始化           使用 USART1、波特率 115200
    USART1_Init(115200);
    USART2_Init(115200);

    printf("System Booting Up...\r\n");

    // [新] 3. 初始化独立看门狗 (IWDG)
    // Tout = ((4 * 2^prer) * rlr) / 40 (ms)
    // prer=4 (分频64), rlr=2500 -> Tout = (256 * 2500) / 40000 = 1600ms
    // 我们设置一个约1.6秒的超时，只要主循环正常运行，就不会复位
    IWDG_Init(4, 2500);
    printf("IWDG Initialized.\r\n");
    
    // ModBUS传感器初始化	    使用 UART3、波特率 9600
    ModBUS_Init();
    // ds18b20传感器初始化
    DS18B20_InitAll();
    // dht11传感器初始化
    // [修改] 将阻塞的初始化改为非阻塞
    while (DHT11_Init())
    {
        printf("DHT11 Init Failed, retrying...\r\n");
        delay_ms(500); // 启动时的延时可以接受
    }
    // 其他硬件初始化...
    Servo_Init();
    Fan_TIM2_PWM_Init();
    Buzzer_Init();
    LED_Init();
    EXTI_KEY_Init();
    Relay_Init();
    WaterPump_And_Heater_Init();
    TIM4_MainTick_Init(300);
    srand(time(NULL));
    System_CloseAll();
    
    // MQTT初始化
    while(!Robust_Initialize_And_Connect_MQTT()) {
        printf("MQTT Init Failed, retrying in 5s...\r\n");
        delay_ms(5000); // 启动时的延时可以接受
    }
    MQTT_Subscribe_All_Topics();
    
    // 屏幕初始化
    Lcd_Init();
    Lcd_Clear(GRAY0); 
    Gui_DrawFont_GBK16(5, 2, BLUE, GRAY0, "御霜塔-霜冻预警"); 
    Gui_DrawLine(0, 20, 139, 20, GRAY1);
    Gui_DrawFont_GBK16(5-4, 22, BLACK, GRAY0, "T1:");
    Gui_DrawFont_GBK16(69-4, 22, BLACK, GRAY0, "T2:");   
    Gui_DrawFont_GBK16(5-4, 42, BLACK, GRAY0, "T3:");  
    Gui_DrawFont_GBK16(69-4, 42, BLACK, GRAY0, "T4:");
    Gui_DrawFont_GBK16(0, 62, BLACK, GRAY0, " Amb Temp:");
    Gui_DrawFont_GBK16(5, 82, BLACK, GRAY0, "风速:");
    Gui_DrawFont_GBK16(5, 103, BLACK, GRAY0, "湿度:");
    
    //4. 定义任务调度的时间戳变量
    uint64_t lastSensorReadTime = 0;
    uint64_t lastMqttReportTime = 0;
    uint64_t lastMqttKeepaliveTime = 0;

    //声明一个静态变量来存储上一个周期的干预状态
    static InterventionMethod_t previous_Intervention_Method = INTERVENTION_NONE;

    while (1)
    {
        //喂狗，表明主循环仍在正常运行
        IWDG_Feed();

        //最高优先级检查: 立即检查MQTT连接丢失标志位
        if (g_mqtt_connection_lost_flag)
        {
            printf("CRITICAL: Main loop detected connection loss flag. Initiating immediate reconnect...\r\n");
            
            // 尝试重连
            MQTT_Check_And_Reconnect();
            
            // **关键**: 不论重连是否成功，都必须清除标志位，以避免无限重连循环。
            // 如果重连失败，下一次通信尝试会再次将它置位。
            g_mqtt_connection_lost_flag = false;
            
            // 跳过本次循环的其余部分，从头开始新的循环。
            // 这可以防止在刚尝试重连后，立即执行那些依赖网络且可能再次失败的任务。
            continue; 
        }

        //获取当前系统时间
        uint64_t currentTime = System_GetTimeMs();

        // --- 任务1: 周期性处理串口接收 (非阻塞) ---
        Handle_Serial_Reception();

        // --- 任务2: 周期性读取传感器数据 ---
        if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL_MS)
        {
            lastSensorReadTime = currentTime; // 更新上次执行时间
            
            read_all_environmental_data(&env_data);
            Display_All_Data(&env_data); // 实时更新本地显示
            
            // 传感器数据读取完毕，进行决策和控制
            Crop_Critical_Temp = get_critical_temp(g_device_status.crop_stage); // 使用云端同步的作物状态
            current_inversion = Analyze_Inversion_Layer(&env_data);
            Intervention_Method = Determine_Optimal_Intervention(&current_inversion, &SysAbilities, &env_data, Crop_Critical_Temp);

            //检查系统是否从“无需干预”状态切换到“需要干预”的状态
            if (Intervention_Method != INTERVENTION_NONE && previous_Intervention_Method == INTERVENTION_NONE)
            {
                // 系统从“无需干预”切换到“需要干预”，触发报警
                MQTT_Post_Frost_Alert_Event(env_data.ambient_temp);
            }
            //更新上一个状态，为下一次比较做准备
            previous_Intervention_Method = Intervention_Method; 

            // 根据决策执行控制
            switch (Intervention_Method) 
            {
                case INTERVENTION_NONE:
                    System_CloseAll();
                    LED_SetColor(COLOR_GREEN);
                    break;
                case INTERVENTION_SPRINKLERS:
                    Heater_OFF(); Fan_OFF(); Set_Servo_Angle(0);
                    LED_SetColor(COLOR_RED); Buzzer_Alarm(3); Water_Pump_ON();
                    powers.sprinkler_power = calculate_sprinkler_power(&env_data, Crop_Critical_Temp);
                    Sprinkler_Set_Power(powers.sprinkler_power);
                    break;
                case INTERVENTION_FANS_ONLY:
                    Heater_OFF(); Water_Pump_OFF();
                    LED_SetColor(COLOR_RED); Buzzer_Alarm(3); Fan_ON();
                    float target_height_fan = calculate_optimal_intervention_height(&current_inversion);
                    float Angle_fan = calculate_servo_angle(target_height_fan);
                    Set_Servo_Angle((uint16_t)Angle_fan);
                    powers.fan_power = calculate_fan_power(&current_inversion, env_data.wind_speed);
                    Fan_Set_Speed(powers.fan_power);
                    break;
                case INTERVENTION_HEATERS_ONLY:
                    Fan_OFF(); Water_Pump_OFF(); Set_Servo_Angle(0);
                    LED_SetColor(COLOR_RED); Buzzer_Alarm(3); Heater_ON();
                    powers.heater_power = calculate_heater_power(&env_data, Crop_Critical_Temp);
                    Heater_Set_Power(powers.heater_power);
                    break;
                case INTERVENTION_FANS_THEN_HEATERS:
                    Water_Pump_OFF();
                    LED_SetColor(COLOR_RED); Buzzer_Alarm(3);
                    Heater_ON(); Fan_ON();
                    float target_height_mix = calculate_optimal_intervention_height(&current_inversion);
                    float Angle_mix = calculate_servo_angle(target_height_mix);
                    Set_Servo_Angle((uint16_t)Angle_mix);
                    powers.fan_power = calculate_fan_power(&current_inversion, env_data.wind_speed);
                    Fan_Set_Speed(powers.fan_power);
                    powers.heater_power = calculate_heater_power(&env_data, Crop_Critical_Temp);
                    Heater_Set_Power(powers.heater_power);
                    break;
            }

            DATA_Flag = 1; // 标记数据已准备好，可以上报
        }

        // --- 任务3: 周期性上报数据到MQTT服务器 ---
        if (DATA_Flag == 1 && (currentTime - lastMqttReportTime >= MQTT_REPORT_INTERVAL_MS))
        {
            lastMqttReportTime = currentTime; // 更新上次执行时间
            
            printf("Publishing data to OneNET...\r\n");
            system_status.env_data = &env_data;
            system_status.capabilities = &SysAbilities;
            system_status.method = Intervention_Method;
            system_status.Powers = &powers;
            system_status.crop_stage = g_device_status.crop_stage; // 使用全局变量
            
            MQTT_Publish_All_Data_Adapt(&system_status);
            
            DATA_Flag = 0; // 清除标志，等待下一次传感器数据就绪
            MQTT_Disconnect(); 
        }

        // --- 任务4: 周期性检查MQTT连接状态 ---
        if (currentTime - lastMqttKeepaliveTime >= MQTT_KEEPALIVE_INTERVAL_MS)
        {
            lastMqttKeepaliveTime = currentTime;
            MQTT_Check_And_Reconnect(); // 检查并在需要时重连
        }

        //任务5: 环境模拟更新
        if(g_simulation_tick_flag == 1)
        {
            en_count_flag = 0;
            Sim_Update_Environment(&env_data , &powers);
            g_simulation_tick_flag = 0;
        }
    }
}

void read_all_environmental_data(EnvironmentalData_t* data)
{
    // 读取4个DS18B20温度
    for(int i=0; i<4; i++){
        data->temperatures[i] = DS18B20_Get_Temp(i);
    }
    
    // 读取DHT11温湿度
    DHT11_Read_Data(&temperature_temp, &humidity_temp);
    data->humidity = humidity_temp / 10.0f;
    data->ambient_temp = temperature_temp / 10.0f;
    
    // 读取风速
    Get_Wind_Data(&wind_speed, &wind_power);
    data->wind_speed = wind_speed;

    // 从全局状态同步大气压值 (由云端设置)
    data->pressure = g_device_status.pressure;
}

void System_CloseAll(void)
{
    LED_SetColor(COLOR_BLACK);
    Water_Pump_OFF();
    Heater_OFF();
    Fan_OFF();
    Set_Servo_Angle(0);
}

// 中断服务函数：处理外部中断线8和9
void EXTI9_5_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
        high++;
        if(high == 5) high = 0;
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
    
    if(EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        DATA_Flag = 1;
        EXTI_ClearITPendingBit(EXTI_Line9);
    }
}

void EXTI15_10_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        DATA_Flag = 0;
        System_CloseAll();
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
}

void TIM4_IRQHandler(void) 
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) 
    {
        if(en_count_flag == 0)
        {
            g_simulation_tick_flag = 1;
            en_count++;
            en_count_flag = 1;
        }
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}