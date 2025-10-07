#ifndef __ONENET_MQTT_H
#define __ONENET_MQTT_H

#include <stdbool.h> // 引入布尔类型
#include <stdint.h>  // 引入标准整型
#include "Frost_Detection.h" // 确保包含了 SystemStatus_t 的定义
/*
 ===============================================================================
                            1. 用户配置区域
 ===============================================================================
*/

// --- 设备三元组 ---
#define MQTT_PRODUCT_ID         "30w1g93kaf"
#define MQTT_DEVICE_NAME        "Yushuang_Tower_007"
#define MQTT_PASSWORD_SIGNATURE "version=2018-10-31&res=products%2F30w1g93kaf%2Fdevices%2FYushuang_Tower_007&et=1790671501&method=md5&sign=F48CON9W%2FTkD6dPXA%2FKxgQ%3D%3D"

// --- 风扇功率控制参数 ---
#define FAN_MIN_POWER    20  // 最小功率 (%)
#define FAN_MAX_POWER    80  // 最大功率 (%)
#define FAN_BASE_POWER   50  // 基础功率 (%)


/*
 ===============================================================================
                            2. 公共数据结构与全局变量
 ===============================================================================
*/

/**
 * @brief 统一存储所有设备属性的当前状态
 */
typedef struct {
    // 温度数据
    float  temp1;
    float  temp2;
    float  temp3;
    float  temp4;
    // 环境数据
    float  ambient_temp;
    float  humidity;
    float  pressure;
    float  wind_speed;
    // 系统状态
    int intervention_status;
    // 设备可用性
    bool sprinklers_available;
    bool fans_available;
    bool heaters_available;
    // 作物生长阶段
    int crop_stage;
    // 风扇功率控制
    int fan_power; // 风扇当前功率 (%)
} DeviceStatus;

// 声明一个全局的设备状态实例，供其他文件访问
extern DeviceStatus g_device_status;


/*
 ===============================================================================
                            3. 公开函数原型
 ===============================================================================
*/
bool Robust_Initialize_And_Connect_MQTT(void);
bool MQTT_Subscribe_All_Topics(void);
void MQTT_Get_Desired_Crop_Stage(void);
void MQTT_Post_Frost_Alert_Event(float current_temp);
void MQTT_Publish_All_Data(const DeviceStatus* status);
void Handle_Serial_Reception(void);

void MQTT_Publish_All_Data_Adapt(const SystemStatus_t* system_status);

#endif // __ONENET_MQTT_H
