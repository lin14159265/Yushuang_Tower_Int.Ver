#include "Frost_Detection.h"
#include "stdio.h"
#include "math.h"
// 传感器高度数组
static const float SENSOR_HEIGHTS[4] = {HEIGHT_0, HEIGHT_1, HEIGHT_2, HEIGHT_3};

float es_water(float T) 
{
    return E0 * exp(A_WATER * T / (T + B_WATER));
}


float es_ice(float T) 
{
    return E0 * exp(A_ICE * T / (T + B_ICE));
}


float calculate_saturation_vapor_pressure(float temperature) 
{
    if (temperature >= 0) 
    {
        return es_water(temperature);
    } 
    else 
    {
        return es_ice(temperature);
    }
}


float calculate_dew_point(float temperature, float relative_humidity) 
{
    // 1. 计算实际水汽压 (e)
    float es = es_water(temperature);
    float e = es * relative_humidity / 100.0;
    
    // 2. 使用Magnus公式反算露点温度
    // 露点是水汽凝结成“露”（液态水）的温度，因此必须使用水面公式的常数反算
    float log_e_ratio = log(e / E0);
    float numerator = B_WATER * log_e_ratio;
    float denominator = A_WATER - log_e_ratio;
    
    return numerator / denominator;
}


float calculate_wet_bulb_temp(float dry_bulb_temp, float relative_humidity, float pressure_hPa) 
{
    // 1. 计算实际水汽压 (e)
    float e = es_water(dry_bulb_temp) * relative_humidity / 100.0;
    
    // 2. 计算湿球常数 (gamma)
    float gamma = GAMMA_FACTOR * pressure_hPa;
    
    // 3. 迭代求解湿球温度 (Tw)
    float dew_point = calculate_dew_point(dry_bulb_temp, relative_humidity);
    float wet_bulb_temp = (dry_bulb_temp + dew_point) / 2.0;
    
    int max_iterations = 100;
    float tolerance = 0.001;
    
    for (int i = 0; i < max_iterations; ++i) 
    {
        float es_wet;
        float derivative;

        if (wet_bulb_temp >= 0) 
        {
            es_wet = es_water(wet_bulb_temp);
            derivative = (A_WATER * B_WATER * es_wet) / ((wet_bulb_temp + B_WATER) * (wet_bulb_temp + B_WATER));
        } 
        else 
        {
            es_wet = es_ice(wet_bulb_temp);
            derivative = (A_ICE * B_ICE * es_wet) / ((wet_bulb_temp + B_ICE) * (wet_bulb_temp + B_ICE));
        }
        
        float f_Tw = es_wet - gamma * (dry_bulb_temp - wet_bulb_temp) - e;
        float f_prime_Tw = derivative + gamma;

        if (fabs(f_prime_Tw) < 1e-9) 
        {
            break; 
        }

        float new_wet_bulb = wet_bulb_temp - f_Tw / f_prime_Tw;
        
        if (fabs(new_wet_bulb - wet_bulb_temp) < tolerance) 
        {
            return new_wet_bulb;
        }
        
        wet_bulb_temp = new_wet_bulb;
        
    }
    printf("wet_bulb_temp :%f\r\n",wet_bulb_temp);
    return wet_bulb_temp;
}


// 分析逆温层结构
InversionLayerInfo_t Analyze_Inversion_Layer(EnvironmentalData_t* env_data) 
{
    InversionLayerInfo_t info = {0};
    
    // 1. 计算各层温度梯度
    float gradients[3];
    for (int i = 0; i < 3; i++) 
    {
        float height_diff = SENSOR_HEIGHTS[i+1] - SENSOR_HEIGHTS[i];
        gradients[i] = (env_data->temperatures[i+1] - env_data->temperatures[i]) / height_diff;
    }
    
    // 2. 寻找连续的逆温层
    int inversion_start = -1;
    int inversion_end = -1;
    float max_gradient = 0.0f;
    float total_gradient = 0.0f;
    int inversion_count = 0;
    
    for (int i = 0; i < 3; i++) 
    {
        if (gradients[i] > INVERSION_GRADIENT_THRESHOLD) 
        {
            if (inversion_start == -1) inversion_start = i;
            inversion_end = i;
            max_gradient = fmaxf(max_gradient, gradients[i]);
            total_gradient += gradients[i];
            inversion_count++;
        } 
        else if (inversion_start != -1) 
        {
            break; // 逆温层中断
        }
    }
    
    // 3. 判断有效逆温层（至少连续两层）
    if (inversion_count >= 2 && inversion_start != -1) 
    {
        info.is_valid = 1;
        info.strength = total_gradient / inversion_count;
        info.base_height = SENSOR_HEIGHTS[inversion_start];
        info.top_height = SENSOR_HEIGHTS[inversion_end + 1];
        info.thickness = info.top_height - info.base_height;
        
        // 计算风险等级
        if (info.strength < 1.0f) info.risk_level = 1;
        else if (info.strength < 2.0f) info.risk_level = 2;
        else info.risk_level = 3;
    }
    
    return info;
}

// 确定最优干预方法
InterventionMethod_t Determine_Optimal_Intervention(InversionLayerInfo_t* inversion, SystemCapabilities_t* capabilities, EnvironmentalData_t* env_data, float critical_temp)
{
    float min_temp = env_data->temperatures[0];
    
    //  预检查：如果温度远高于安全线，则无需任何干预
    if (min_temp > (critical_temp + INTERVENTION_SAFETY_MARGIN))
    {
        return INTERVENTION_NONE;
    }

    // 2. 判断霜冻类型：是否是适合风扇工作的辐射霜冻条件
    uint8_t is_radiation_frost_condition = (env_data->wind_speed < (WIND_SPEED_NO_INTERVENTION)  && inversion->is_valid);

    // 辐射霜冻策略 (风扇优先)
    // 如果是辐射霜冻条件，并且系统拥有风扇，则优先执行风扇策略
    if (is_radiation_frost_condition && capabilities->fans_available)
    {
        float current_dew_point = calculate_dew_point(min_temp, env_data->humidity);

        // 检查是否为严重霜冻，需要组合干预
        if (current_dew_point < (critical_temp - SEVERE_FROST_MARGIN) || min_temp < (critical_temp - SEVERE_FROST_MARGIN))
        {
            if (capabilities->heaters_available)
            {
                return INTERVENTION_FANS_THEN_HEATERS; // 严重霜冻，风扇+加热
            }
        }
        
        // 对于中等风险的辐射霜冻，仅用风扇是最高效的
        return INTERVENTION_FANS_ONLY;
    }
    
    // 平流霜冻 或 无风扇/无效逆温 策略 (洒水/加热器备用)
    // 只有在风扇策略不适用时，才考虑洒水和加热
    else
    {
        // 检查洒水系统是否可用且适用
        if (capabilities->sprinklers_available)
        {
            float wet_bulb_temp = calculate_wet_bulb_temp(min_temp, env_data->humidity, env_data->pressure);
            if (wet_bulb_temp <= critical_temp)
            {
                // 洒水前必须检查风速风险
                if (env_data->wind_speed <= WIND_SPEED_SPRINKLERS_RISKY)
                {
                    return INTERVENTION_SPRINKLERS;
                }
            }
        }
        
        //  如果洒水策略不满足条件（或不可用），最后的选择是加热器
        if (capabilities->heaters_available)
        {
            return INTERVENTION_HEATERS_ONLY;
        }
    }
    
    // 如果所有策略都不满足或不可用，则不进行任何操作
    return INTERVENTION_NONE;
}


// 计算最佳干预高度
float calculate_optimal_intervention_height(InversionLayerInfo_t* inversion) 
{
    if (!inversion->is_valid) 
    {
        return HEIGHT_1; // 默认高度
    }
    // 瞄准逆温层的中上部
    return inversion->base_height + (inversion->thickness * 0.7f);
}



//根据逆温层信息的风机功率计算
uint8_t calculate_fan_power(InversionLayerInfo_t* risk_info, float wind_speed) 
{
    if (!risk_info->is_valid) 
    {
        return 0;
    }
    
    // 逆温强度因子 (0.0~1.0)
    float inversion_factor = risk_info->strength / MAX_INVERSION_STRENGTH;
    if (inversion_factor > 1.0f) inversion_factor = 1.0f;
    
    // 逆温层厚度因子 (0.0~1.0)
    float thickness_factor = risk_info->thickness / 4.0f;
    if (thickness_factor > 1.0f) thickness_factor = 1.0f;
    
    // 风力不足因子 (0.0~1.0)
    float wind_deficit_factor = 1.0f - (wind_speed / WIND_SPEED_NO_INTERVENTION);
    if (wind_deficit_factor < 0.0f) wind_deficit_factor = 0.0f;
    
    // 修正：使用 MAX_POWER 作为基准
    float power_float = MAX_POWER * inversion_factor * thickness_factor * wind_deficit_factor;
    
    // 只需要应用下限（上限已经由各个因子保证）
    if (power_float < MIN_POWER) 
    {
        power_float = MIN_POWER;
    }
    
    return (uint8_t)power_float;
}

  
//模拟加热功率计算   
uint8_t calculate_heater_power(EnvironmentalData_t* env_data, float critical_temp)
{
    float min_temp = env_data->temperatures[0];

    float upper_temp_bound = critical_temp + INTERVENTION_SAFETY_MARGIN;
    float lower_temp_bound = critical_temp - SEVERE_FROST_MARGIN;

    // 如果温度高于安全区，则因子为0
    if (min_temp >= upper_temp_bound) 
    {
        return 0;
    }

    // 将当前温度映射到0-1的因子
    float temp_factor = (upper_temp_bound - min_temp) / (upper_temp_bound - lower_temp_bound);
    if (temp_factor > 1.0f) temp_factor = 1.0f;
    if (temp_factor < 0.0f) temp_factor = 0.0f;

    // --- 干燥空气风险因子 (0.5 ~ 1.0) ---
    // 露点越低，空气越干，辐射降温越快，需要更大功率
    float dew_point = calculate_dew_point(min_temp, env_data->humidity);
    float dryness_factor = 1.0f - (dew_point / 10.0f); // 简单线性模型，露点每下降10度，因子增加1
    if (dryness_factor < 0.5f) dryness_factor = 0.5f; // 最小为0.5，因为湿度大也需要加热
    if (dryness_factor > 1.5f) dryness_factor = 1.5f; // 设个上限

    // --- 风寒效应因子 (1.0 ~ 2.0) ---
    // 风越大，热量损失越快，需要更大功率
    float wind_chill_factor = 1.0f + (env_data->wind_speed / WIND_SPEED_SPRINKLERS_RISKY); // 风速达到洒水风险上限时，因子为2
    
    // --- 最终功率计算 ---
    // 基础功率由温度决定，再乘以其他风险因子
    float power_float = MAX_POWER * temp_factor * dryness_factor * wind_chill_factor;

    // 应用上下限
    if (power_float > MAX_POWER) power_float = MAX_POWER;
    
    // 只有在需要启动时，才应用最小功率
    if (power_float > 0 && power_float < MIN_POWER) 
    {
        power_float = MIN_POWER;
    }

    return (uint8_t)power_float;
}



uint8_t calculate_sprinkler_power(EnvironmentalData_t* env_data, float critical_temp)
{
    float min_temp = env_data->temperatures[0];

    // --- 湿球温度危险度因子 (0.0 ~ 1.0) ---
    // 湿球温度是决定洒水效果的关键
    float wet_bulb_temp = calculate_wet_bulb_temp(min_temp, env_data->humidity, env_data->pressure);
    
    // 如果湿球温度高于临界值，无需洒水
    if (wet_bulb_temp > critical_temp) 
    {
        return 0;
    }

    // 将湿球温度的危险程度映射到0-1的因子
    // 假设低于-5°C时达到最大危险
    const float WET_BULB_DANGER_ZONE = 5.0f; // (critical_temp 到 critical_temp - 5.0)
    float wb_factor = (critical_temp - wet_bulb_temp) / WET_BULB_DANGER_ZONE;
    if (wb_factor > 1.0f) wb_factor = 1.0f;
    if (wb_factor < 0.0f) wb_factor = 0.0f;

    // --- 风速蒸发加剧因子 (1.0 ~ 2.0) ---
    // 风越大，蒸发越快，需要更大的洒水量来补偿
    // 高层决策已排除了风速>3.0的风险情况，这里只处理3.0以下的风
    float wind_factor = 1.0f + (env_data->wind_speed / WIND_SPEED_SPRINKLERS_RISKY);

    // --- 最终功率计算 ---
    float power_float = MAX_POWER * wb_factor * wind_factor;

    // 应用上下限
    if (power_float > MAX_POWER) power_float = MAX_POWER;

    if (power_float > 0 && power_float < MIN_POWER) 
    {
        power_float = MIN_POWER;
    }
    
    return (uint8_t)power_float;
}

