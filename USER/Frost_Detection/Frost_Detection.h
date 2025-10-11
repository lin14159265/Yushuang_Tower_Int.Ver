#ifndef __FROST_DETECTION_H
#define __FROST_DETECTION_H

#include "sys.h"

// 传感器安装高度定义（单位：米）- 采用近地层加密方案
#define HEIGHT_0 1
#define HEIGHT_1 2 
#define HEIGHT_2 3
#define HEIGHT_3 4

#define E0 0.6108      // 0°C时的饱和水汽压 (kPa)

#define pressure_MIN  870.0
#define pressure_MAX  1083.8

// 水面 (liquid water)
#define A_WATER 17.27
#define B_WATER 237.3

// 冰面 (ice)
#define A_ICE 21.87
#define B_ICE 265.5

// 湿球常数因子 (适用于通风良好的湿球温度计)
// 单位为 K⁻¹ 或 °C⁻¹，当压力P单位为 hPa 时
#define GAMMA_FACTOR 0.000665 

// 阈值定义（基于气象学研究和实际应用）
#define INVERSION_GRADIENT_THRESHOLD 0.5f     // 逆温梯度阈值(°C/m)
#define MAX_INVERSION_STRENGTH 3.0f           // 最大逆温强度 (°C/m)
#define WIND_SPEED_NO_INTERVENTION 2.0f       // 无需干预的风速阈值(m/s)
#define WIND_SPEED_FULL_INTERVENTION 0.5f     // 需要全力干预的风速阈值(m/s)
#define WIND_SPEED_SPRINKLERS_RISKY 3.0f      // 洒水系统风险风速阈值(m/s)
#define INTERVENTION_SAFETY_MARGIN 1.0f       // 安全边际(°C)
#define SEVERE_FROST_MARGIN 2.0f              // 严重霜冻边际(°C)

//功率控制参数
#define MIN_POWER        20    // 最小功率 (%)
#define MAX_POWER        100    // 最大功率 (%) 




// 系统能力配置
typedef struct {
    uint8_t sprinklers_available;  // 是否有洒水系统
    uint8_t fans_available;        // 是否有风扇系统  
    uint8_t heaters_available;     // 是否有加热系统
} SystemCapabilities_t;

// 环境数据结构
typedef struct {
    float temperatures[4];  // 4个高度的温度(°C)
    float humidity;         // 湿度(%)
    float ambient_temp;     // 环境温度(°C) 
    float wind_speed;       // 风速(m/s)
    float pressure;         // 大气压(kPa)
} EnvironmentalData_t;

// 逆温层信息结构
typedef struct {
    float strength;         // 逆温层强度(°C/m)
    float base_height;      // 逆温层底高(m)
    float top_height;       // 逆温层顶高(m) 
    float thickness;        // 逆温层厚度(m)
    uint8_t is_valid;       // 是否有效逆温层
    uint8_t risk_level;     // 风险等级 (0-3)
} InversionLayerInfo_t;

// 干预方法枚举
typedef enum {
    INTERVENTION_NONE,             // 无需干预
    INTERVENTION_SPRINKLERS,       // 仅洒水
    INTERVENTION_FANS_ONLY,        // 仅风扇
    INTERVENTION_HEATERS_ONLY,     // 仅加热
    INTERVENTION_FANS_THEN_HEATERS // 风扇+加热组合
} InterventionMethod_t;

// 干预功率结构体
typedef struct {
    uint8_t fan_power;
    uint8_t heater_power;
    uint8_t sprinkler_power;
} InterventionPowers_t;

//作物时期枚举
typedef enum {
    STAGE_TIGHT_CLUSTER,             // 紧簇期（花蕾紧密聚集阶段）
    STAGE_FULL_BLOOM,                // 盛花期（花朵完全开放阶段）
    STAGE_SMALL_FRUIT,               // 小果期（果实初形成阶段）
    STAGE_MATURATION                 // 成熟期
} CROP_CRITICAL_TEMPERATURE;

//统一的系统状态结构体
typedef struct {
    EnvironmentalData_t* env_data;      // 指向环境数据的指针
    SystemCapabilities_t* capabilities; // 指向系统能力的指针
    InterventionMethod_t method;        // 当前决策的干预方法
    InterventionPowers_t* Powers;        // 指向功率的指针
    int crop_stage;                     // 当前作物生长阶段
} SystemStatus_t;



InversionLayerInfo_t Analyze_Inversion_Layer(EnvironmentalData_t* env_data);
InterventionMethod_t Determine_Optimal_Intervention(InversionLayerInfo_t* inversion, SystemCapabilities_t* capabilities, EnvironmentalData_t* env_data, float critical_temp);
uint8_t calculate_fan_power(InversionLayerInfo_t* risk_info, float wind_speed);
uint8_t calculate_heater_power(EnvironmentalData_t* env_data, float critical_temp);
uint8_t calculate_sprinkler_power(EnvironmentalData_t* env_data, float critical_temp);
float calculate_optimal_intervention_height(InversionLayerInfo_t* inversion);

#endif


