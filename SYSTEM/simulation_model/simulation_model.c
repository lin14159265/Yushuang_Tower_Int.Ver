#include "simulation_model.h"
#include "stdlib.h"
// 模拟参数 (在报告中要说明这些是为了演示而加速的)
#define SIM_NATURAL_COOLING      -0.02f
#define SIM_NATURAL_EFFECT       +0.025f
#define SIM_MIXING_EFFICIENCY    0.30f  // [微调] 基础效率可以略微调高，因为后面会动态降低
#define SIM_HEATER_EFFECT        0.04f
#define SIM_SPRINKLER_TARGET_TEMP 0.0f
#define SIM_SPRINKLER_STRENGTH   0.5f
#define SIM_DISTURBANCE_MAGNITUDE 0.01f // [新增] 随机干扰的幅度 (+/- 0.005°C)



void TIM4_MainTick_Init(u16 period_ms)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = period_ms - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 36000 - 1; 
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 优先级可以根据需要调整
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    
    TIM_Cmd(TIM4, ENABLE);
}
// 辅助函数，根据高度值(米)返回最接近的传感器数组索引
// 确保这个函数与你的SENSOR_HEIGHTS定义匹配
static int get_index_from_height(float height)
{
    if (height <= HEIGHT_0) return 0;
    if (height <= HEIGHT_1) return 1;
    if (height <= HEIGHT_2) return 2;
    return 3;
}

void Sim_Update_Environment(EnvironmentalData_t* env_data,InterventionPowers_t* powers)
{
    if(en_count <= 8)
    {
        if(en_count <= 5)
        {
            for (int i = 0; i < 4; i++) 
            {
                //计算随机干扰
                float random_disturbance = ((float)rand() / RAND_MAX - 0.5f) * SIM_DISTURBANCE_MAGNITUDE;
                env_data->temperatures[i] += SIM_NATURAL_COOLING + random_disturbance;
            }
        }
        else if(en_count > 5)
        {
            for (int i = 0; i < 4; i++) 
            {
                // 计算随机干扰
                float random_disturbance = ((float)rand() / RAND_MAX - 0.5f) * SIM_DISTURBANCE_MAGNITUDE;
                env_data->temperatures[i] += SIM_NATURAL_EFFECT + random_disturbance;
            }
        }
    }

    //模拟风扇打破逆温层的效果 
 if (powers->fan_power > 0) 
    {
        InversionLayerInfo_t inversion_info = Analyze_Inversion_Layer(env_data);

        if (inversion_info.is_valid) 
        {
            int cold_index = get_index_from_height(inversion_info.base_height);
            int warm_index = get_index_from_height(inversion_info.top_height);
            
            float actual_temp_diff = env_data->temperatures[warm_index] - env_data->temperatures[cold_index];

            if (actual_temp_diff > 0) 
            {
                // 1. 计算非线性的动态效率
                // 逆温越强(strength越大)，混合效率越低
                float dynamic_efficiency = SIM_MIXING_EFFICIENCY * (1.0f - inversion_info.strength * 0.1f);
                
                // 2. 效率下限保护
                if (dynamic_efficiency < 0.05f) 
                {
                    dynamic_efficiency = 0.05f;
                }
                
                // [修改] 3. 使用动态效率来计算温度转移量
                float temp_transfer = actual_temp_diff * dynamic_efficiency * ((float)powers->fan_power / 100.0f);

                // 精确地在被识别的层之间进行热量转移 (保持不变)
                env_data->temperatures[cold_index] += temp_transfer;
                env_data->temperatures[warm_index] -= temp_transfer;
            }
        }
    }
    //模拟加热器的效果 (普适性升温) ---
    if (powers->heater_power > 0)
    {
        float heater_lift = SIM_HEATER_EFFECT * ((float)powers->heater_power / 100.0f);
        for (int i = 0; i < 4; i++)
        {
            env_data->temperatures[i] += heater_lift;
        }
    }
    //模拟洒水的效果 (将地面温度稳定在0°C) ---
    if (powers->sprinkler_power > 0) 
    {
        // 洒水只影响近地面温度
        float temp_diff_to_zero = SIM_SPRINKLER_TARGET_TEMP - env_data->temperatures[0];
        
        // 只有当温度低于0度时，洒水的相变潜热才有升温效果
        if (temp_diff_to_zero > 0) 
        {
            // 将温度向0度拉近，效果与功率和温差有关
            float sprinkler_lift = temp_diff_to_zero * SIM_SPRINKLER_STRENGTH * ((float)powers->sprinkler_power / 100.0f);
            env_data->temperatures[0] += sprinkler_lift;
        }
    }
    
    
}

