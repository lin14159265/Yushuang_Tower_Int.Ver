#include "onenet_mqtt.h"
#include <stdint.h>
#include <stm32f10x.h>
#include "Frost_Detection.h"
#include "stm32f10x_conf.h"
#include "UART_DISPLAY.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "delay.h"
#include "Relay.h"
#include "fan.h"
/*
 ===============================================================================
                            模块内部变量与宏定义
 ===============================================================================
*/
#define CMD_BUFFER_SIZE   4096
#define JSON_PAYLOAD_SIZE 4096

// 内部静态全局变量，对外部文件隐藏
static char g_cmd_buffer[CMD_BUFFER_SIZE];
static char g_json_payload[JSON_PAYLOAD_SIZE]; 
static unsigned int g_message_id = 0;
/*
int g_crop_stage = 0;           // 作物生长时期 (默认为0)
int g_intervention_status = 0;  // 人工干预状态 (默认为0)
*/
// 回复类型枚举，仅在模块内部使用
typedef enum {
    REPLY_TO_PROPERTY_SET,
    REPLY_TO_SERVICE_INVOKE
} ReplyType;

// 定义并初始化全局设备状态结构体
DeviceStatus g_device_status = {
    .temp1 = 0.0f,
    .temp2 = 0.0f,
    .temp3 = 0.0f,
    .temp4 = 0.0f,
    .ambient_temp = 0.0f,
    .humidity = 0.0f,
    .pressure = 0.0f,
    .wind_speed = 0.0f,
    .intervention_status = 0,
    .crop_stage = 0,
    .fan_power = 50,
    .sprinklers_available = 1,
    .fans_available = 1,
    .heaters_available = 1
};


/*
 ===============================================================================
                            公开函数实现
 ===============================================================================
*/


/**
 * @brief  发送AT指令并等待响应
 * @param  cmd: 要发送的AT指令字符串。
 * @param  expected_response: 期望在模块的回复中找到的关键字字符串。
 * @param  timeout_ms: 等待响应的超时时间，单位毫秒。
 * @return bool: true 代表成功，false 代表失败。
 */
static bool MQTT_Send_AT_Command(const char* cmd, const char* expected_response, uint32_t timeout_ms)
{
    // 步骤1：清空串口接收缓冲区
    memset(xUSART.USART1ReceivedBuffer, 0, sizeof(xUSART.USART1ReceivedBuffer));
    xUSART.USART1ReceivedNum = 0;

    // 步骤2：通过串口发送AT指令
    printf("SEND: %s", cmd);
    USART1_SendString((char*)cmd);

    // 步骤3：使用SysTick获取当前时间作为超时判断的起点
    uint64_t start_time = System_GetTimeMs();

    // 步骤4：在超时时间内循环等待
    while ((System_GetTimeMs() - start_time) < timeout_ms)
    {
        // 检查串口是否收到了数据
        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';

            // 检查收到的数据中是否包含期望的响应
            if (strstr((char*)xUSART.USART1ReceivedBuffer, expected_response) != NULL)
            {
                printf("SUCCESS: Found response '%s'\r\n\r\n", expected_response);
                return true; // 成功！
            }
        }
        delay_ms(10); 
    }

    // 如果循环结束，说明已经超过了指定的 timeout_ms
    printf("FAIL: Timeout. Did not receive '%s' in %lu ms.\r\n\r\n", expected_response, timeout_ms);
    printf("Last received data: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
    return false; // 失败！
}



/**
 * @brief 使用同步发送-确认机制，可靠地初始化模块并连接到MQTT服务器
 * @return bool: true 代表所有步骤都成功，false 代表有任何一步失败。
 */
bool Robust_Initialize_And_Connect_MQTT(void)
{
    // 1. 检查AT指令是否响应
    if (!MQTT_Send_AT_Command("AT\r\n", "OK", 500)) 
        return false;
    
    // 2. 获取SIM卡信息 (IMSI)
    if (!MQTT_Send_AT_Command("AT+CIMI\r\n", "OK", 1000)) 
        return false;

    // 3. 设置GPRS附着
    if (!MQTT_Send_AT_Command("AT+CGATT=1\r\n", "OK", 1000)) 
        return false;

    // 4. 查询GPRS附着状态，期望回复是 "+CGATT: 1"
    if (!MQTT_Send_AT_Command("AT+CGATT?\r\n", "+CGATT: 1", 3000)) 
        return false;

    // 5. 配置MQTT协议版本为 3.1.1
    if (!MQTT_Send_AT_Command("AT+QMTCFG=\"version\",0,4\r\n", "OK", 1000)) 
        return false;

    // 6. 打开MQTT网络 (连接到OneNET服务器)
    sprintf(g_cmd_buffer, "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n");
    if (!MQTT_Send_AT_Command(g_cmd_buffer, "+QMTOPEN: 0,0", 5000)) 
        return false; 

    // 7. 使用三元组连接MQTT Broker
    sprintf(g_cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", MQTT_DEVICE_NAME, MQTT_PRODUCT_ID, MQTT_PASSWORD_SIGNATURE);
    if (!MQTT_Send_AT_Command(g_cmd_buffer, "+QMTCONN: 0,0,0", 5000)) 
        return false; 
    return true; // 所有步骤都成功了！
}



/**
 * @brief 上报霜冻风险警告事件
 * @param current_temp: 触发告警时的当前温度
 */
void MQTT_Post_Frost_Alert_Event(float current_temp)
{
    char json_payload[256]; 
    g_message_id++;

    sprintf(json_payload, 
        "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":{"
        "\"frost_alert\":{\"value\":{\"current_temp\":%.1f}}"
        "}}",
        g_message_id,
        current_temp
    );

    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/event/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            json_payload);

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}



/**
 * @brief 主动获取云端存储的“作物生长时期”属性的期望值
 * @note  此函数仅负责发送“获取请求”。云平台的结果会通过一条新的 +QMTRECV 消息返回，
 *        其 Topic 为 "$sys/.../thing/property/desired/get/reply"。
 */
 
void MQTT_Get_Desired_Crop_Stage(void)
{
    char json_payload[256]; 
    g_message_id++;

    // params 是一个只包含字符串 "crop_stage" 的数组
    sprintf(json_payload, 
        "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":[\"crop_stage\"]}",
        g_message_id
    );

    // Topic 必须使用 'thing/property/desired/get'
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/desired/get\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            json_payload);

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}


/**
 * @brief  订阅云端下发命令的主题，并等待确认
 * @return bool: true 代表订阅成功, false 代表失败
 */
static bool MQTT_Subscribe_Command_Topic(void)
{
    // 目标Topic: $sys/{product_id}/{device_name}/cmd/request/+
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/cmd/request/+\",1\r\n", 
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME);
            
    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}


/**
 * @brief 订阅“属性设置”的主题，并等待确认
 * @return bool: true 代表订阅成功, false 代表失败
 */
static bool MQTT_Subscribe_Property_Set_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/property/set
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/property/set\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);

    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}

/**
 * @brief  订阅“服务调用”的主题
 * @return bool: true 代表订阅成功, false 代表失败
 */
static bool MQTT_Subscribe_Service_Invoke_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/service/+/invoke
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/service/+/invoke\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);

    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}

/**
 * @brief  订阅“属性获取”的主题，并等待确认
 * @return bool: true 代表订阅成功, false 代表失败
 */
static bool MQTT_Subscribe_Property_Get_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/property/get
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/property/get\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);

    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}



/**
 * @brief  订阅“获取期望属性”的回复主题
 * @return bool: true 代表订阅成功, false 代表失败
 */
static bool MQTT_Subscribe_Desired_Property_Get_Reply_Topic(void)
 {
     // 回复主题的官方格式为: $sys/{product_id}/{device_name}/thing/property/desired/get/reply
     sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/property/desired/get/reply\",1\r\n",
             MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
 
     // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
     return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
 }


/**
 * @brief  一次性订阅所有需要接收消息的主题，并检查每一步的结果
 * @return bool: true 代表所有主题都订阅成功, false 代表有任何一个失败
 */
bool MQTT_Subscribe_All_Topics(void)
{
    printf("INFO: Subscribing to all topics...\r\n");

    if (!MQTT_Subscribe_Command_Topic()) {
        printf("ERROR: Failed to subscribe to Command Topic.\r\n");
        return false;
    }
    
    if (!MQTT_Subscribe_Property_Set_Topic()) {
        printf("ERROR: Failed to subscribe to Property Set Topic.\r\n");
        return false;
    }
    
    if (!MQTT_Subscribe_Service_Invoke_Topic()) {
        printf("ERROR: Failed to subscribe to Service Invoke Topic.\r\n");
        return false;
    }

    if (!MQTT_Subscribe_Property_Get_Topic()) {
        printf("ERROR: Failed to subscribe to Property Get Topic.\r\n");
        return false;
    }
    
    if (!MQTT_Subscribe_Desired_Property_Get_Reply_Topic()) {
        printf("ERROR: Failed to subscribe to Desired Property Get Reply Topic.\r\n");
        return false;
    }

    printf("INFO: All topics subscribed successfully.\r\n\r\n");
    return true; 
}




/**
 * @brief  使用“数据模式”发送MQTT消息
 * @param topic:   要发布到的主题
 * @param payload: 要发送的JSON负载 (注意：是干净的JSON，不带C语言转义符)
 * @return bool:   true 代表发布成功, false 代表失败
 * @note   此函数使用 AT+QMTPUB 的“提示符”模式，先发送指令头，等待模块返回">"，
 *         然后再发送数据负载。这是发送较长或包含特殊字符数据的最稳定方法。
 */
 /*
static bool MQTT_Publish_Message_Prompt_Mode(const char* topic, const char* payload)
{
    // 1. 计算负载的长度
    size_t payload_len = strlen(payload);

    // 2. 构建第一部分AT指令，包含主题和数据长度
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, "AT+QMTPUB=0,0,0,0,\"%s\",%u\r\n", topic, payload_len);

    // 3. 发送指令头，并等待模块返回数据输入提示符 ">"
    //    这是一个关键的握手步骤。我们给它1秒的超时时间。
    if (!MQTT_Send_AT_Command(g_cmd_buffer, ">", 1000))
    {
        printf("ERROR: Did not receive prompt '>' from module for publishing.\r\n");
        return false;
    }

    // 4. 成功收到了 ">"，现在直接发送原始的 payload 数据
    //    注意：这里我们不再使用 MQTT_Send_AT_Command，因为我们不需要等待特定的回复，
    //    而是要等待最终的发布确认 "+QMTPUB: 0,0,0"。
    printf("SEND_PAYLOAD: %s\r\n", payload);
    USART1_SendString((char*)payload);

    // 5. 发送完数据后，模块会进行网络操作，并最终返回发布结果。
    //    我们等待 "+QMTPUB: 0,0,0" 作为成功的标志。网络操作需要更长的时间。
    if (!MQTT_Send_AT_Command("\r\n", "+QMTPUB: 0,0,0", 5000)) // 发送一个换行符触发模块响应
    {
        printf("ERROR: Did not receive publish confirmation after sending payload.\r\n");
        return false;
    }

    printf("INFO: Message on topic '%s' published successfully using prompt mode.\r\n", topic);
    return true;
}
*/






/**
 * @brief 根据回复类型生成不同的JSON
 */
static bool MQTT_Send_Reply(const char* request_id, ReplyType reply_type, const char* identifier, int code, const char* msg)
{
    char reply_topic[128];
    char clean_json_payload[256];

    // 根据回复类型，智能构建JSON
    if (reply_type == REPLY_TO_PROPERTY_SET) {
        // 属性设置的回复，【不带】data字段
        snprintf(clean_json_payload, sizeof(clean_json_payload),
                 "{\"id\":\"%s\",\"code\":%d,\"msg\":\"%s\"}",
                 request_id, code, (code == 200) ? "success" : msg);
    } else { // 适用于 REPLY_TO_SERVICE_INVOKE
        // 服务调用的回复
        snprintf(clean_json_payload, sizeof(clean_json_payload),
                 "{\"id\":\"%s\",\"code\":%d,\"msg\":\"%s\",\"data\":{}}",
                 request_id, code, (code == 200) ? "success" : msg);
    }

    //Topic构建部分
    switch (reply_type)
    {
        case REPLY_TO_PROPERTY_SET:
            snprintf(reply_topic, sizeof(reply_topic),
                     "$sys/%s/%s/thing/property/set_reply",
                     MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
            break;
        case REPLY_TO_SERVICE_INVOKE:
            if (identifier == NULL || identifier[0] == '\0') {
                return false;
            }
            // 动态构建包含 identifier 的回复Topic
            snprintf(reply_topic, sizeof(reply_topic),
                     "$sys/%s/%s/thing/service/%s/invoke_reply",
                     MQTT_PRODUCT_ID, MQTT_DEVICE_NAME, identifier);
            break;
        default:
            return false;
    }

    //AT指令发送部分
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
             "AT+QMTPUB=0,0,0,0,\"%s\",\"%s\"\r\n",
             reply_topic, clean_json_payload);

    return MQTT_Send_AT_Command(g_cmd_buffer, "OK", 5000);
}

/**
 * @brief 处理“属性获取”请求，并回复当前设备状态
 * @param request_id: 请求的唯一标识符
 * @param params_str: 请求中包含的参数字符串 (JSON格式)
 * @return bool: true 代表回复成功, false 代表失败
 * @note  此函数动态构建 data 对象，仅包含请求中指定的属性，避免冗余数据。
 */
static bool MQTT_Reply_To_Property_Get_Refactored(const char* request_id, const char* params_str)
{
    static char data_payload[4096] = {0};
    static char final_json[2048] = {0};
    static char reply_topic[256] = {0};

    //构建 data 对象
    char* p = data_payload;
    size_t remaining_len = sizeof(data_payload);
    int written_len = 0;
    bool first_item_added = false; // 用于控制逗号

    // 写入起始的 '{'
    written_len = snprintf(p, remaining_len, "{");
    p += written_len;
    remaining_len -= written_len;

    // 宏定义一个帮助函数，减少重复代码
    // __VA_ARGS__ 用于处理可变参数
    #define ADD_PROPERTY(param_name, format, ...) \
    if (remaining_len > 1 && strstr(params_str, "\"" param_name "\"") != NULL) { \
        if (first_item_added) { \
            written_len = snprintf(p, remaining_len, ","); \
            p += written_len; \
            remaining_len -= written_len; \
        } \
        written_len = snprintf(p, remaining_len, "\"" param_name "\":" format, __VA_ARGS__); \
        p += written_len; \
        remaining_len -= written_len; \
        first_item_added = true; \
    }

    // 使用宏来添加各个属性
    ADD_PROPERTY("ambient_temp", "%.1f", g_device_status.ambient_temp);
    ADD_PROPERTY("humidity", "%.1f", g_device_status.humidity);
    ADD_PROPERTY("crop_stage", "%d", g_device_status.crop_stage);
    ADD_PROPERTY("wind_speed", "%.1f", g_device_status.wind_speed);
    ADD_PROPERTY("temp1", "%.1f", g_device_status.temp1);
    ADD_PROPERTY("temp2", "%.1f", g_device_status.temp2);
    ADD_PROPERTY("temp3", "%.1f", g_device_status.temp3);
    ADD_PROPERTY("temp4", "%.1f", g_device_status.temp4);
    ADD_PROPERTY("pressure", "%.1f", g_device_status.pressure);

    ADD_PROPERTY("sprinklers_available", "%d", g_device_status.sprinklers_available);
    ADD_PROPERTY("fans_available", "%d", g_device_status.fans_available);
    ADD_PROPERTY("heaters_available", "%d", g_device_status.heaters_available);

    ADD_PROPERTY("intervention_status", "%d", g_device_status.intervention_status);
    ADD_PROPERTY("fan_power", "%d", g_device_status.fan_power);
    ADD_PROPERTY("heater_power", "%d", g_device_status.heater_power);
    ADD_PROPERTY("sprinkler_power", "%d", g_device_status.sprinkler_power);

    if (remaining_len > 1) {
        snprintf(p, remaining_len, "}");
    } else {
        data_payload[sizeof(data_payload) - 2] = '}';
        data_payload[sizeof(data_payload) - 1] = '\0';
    }

    // --- 构建完整的回复JSON ---
    snprintf(final_json, sizeof(final_json),
        "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":%s}",
        request_id,
        data_payload);

    // --- 构建回复Topic ---
    snprintf(reply_topic, sizeof(reply_topic),
        "$sys/%s/%s/thing/property/get_reply",
        MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);

    // --- 构建并发送AT指令 ---
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE,
             "AT+QMTPUB=0,0,0,0,\"%s\",\"%s\"\r\n",
             reply_topic, final_json);

    return MQTT_Send_AT_Command(g_cmd_buffer, "OK", 5000);

}

/**
 * @brief  从一个JSON格式的字符串中查找指定的键(key)，并解析其对应的字符串值。
 * @param buffer: 包含JSON内容的字符串。
 * @param key:    要查找的JSON键名。
 * @param result: 如果解析成功，字符串值将被存放在这个缓冲区中。
 * @param max_len: result缓冲区的最大长度，防止溢出。
 * @return int:   如果成功找到并解析了字符串，返回1；否则返回0。
 */
static int find_and_parse_json_string(const char* buffer, const char* key, char* result, int max_len)
{
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);

    char* p_key = strstr(buffer, search_key);
    if (p_key == NULL) return 0;

    char* p_val_start = p_key + strlen(search_key);

    while (*p_val_start && isspace((unsigned char)*p_val_start)) p_val_start++;
    if (*p_val_start != '\"') return 0;
    p_val_start++;

    char* p_val_end = strchr(p_val_start, '\"');
    if (p_val_end == NULL) return 0;

    int val_len = p_val_end - p_val_start;
    if (val_len >= max_len) val_len = max_len - 1;

    memcpy(result, p_val_start, val_len);
    result[val_len] = '\0';

    return 1;
}


/**
 * @brief  从一个JSON格式的字符串中查找指定的键(key)，并解析其对应的整数值。
 * @param buffer: 包含JSON内容的字符串。
 * @param key:    要查找的JSON键名。
 * @param result: 如果解析成功，整数值将被存放在这个指针指向的地址。
 * @return int:   如果成功找到并解析了整数，返回1；否则返回0。
 */
static int find_and_parse_json_int(const char* buffer, const char* key, int* result)
{
    char search_key[64];
    // 1. 构造要搜索的键格式
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    // 2. 查找键的位置
    char* p_key = strstr(buffer, search_key);
    if (p_key == NULL) {
        return 0; // 没找到键
    }

    // 3. 移动指针到键的末尾
    char* p_val = p_key + strlen(search_key);

    // 4. 跳过键和冒号之间的所有空格
    while (*p_val && isspace((unsigned char)*p_val)) {
        p_val++;
    }

    // 5. 检查后面是否是冒号
    if (*p_val != ':') {
        return 0; // 格式错误，键后面不是冒号
    }
    p_val++; // 跳过冒号

    // 6. 使用strtol进行安全的字符串到长整型转换
    char* end_ptr;
    long parsed_value = strtol(p_val, &end_ptr, 10);

    // 7. 验证转换是否成功
    // 如果p_val和end_ptr指向同一个地址，说明冒号后面第一个字符就不是数字，转换失败。
    if (p_val == end_ptr) {
        return 0; // 转换失败
    }

    // 8. 如果成功，将结果存入result指针
    *result = (int)parsed_value;
    return 1; // 成功
}


static int find_and_parse_json_float(const char* buffer, const char* key, float* result)
{
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    char* p_key = strstr(buffer, search_key);
    if (p_key == NULL) return 0;

    char* p_val = p_key + strlen(search_key);
    while (*p_val && isspace((unsigned char)*p_val)) p_val++;
    if (*p_val != ':') return 0;
    p_val++;

    char* end_ptr;
    // 使用 strtof 函数将字符串转换为 float
    float parsed_value = strtof(p_val, &end_ptr);

    if (p_val == end_ptr) return 0; // 转换失败

    *result = parsed_value;
    return 1; // 成功
}


/**
 * @brief 统一处理所有从云平台接收到的MQTT消息，并增加回复状态检查
 * @param buffer: 指向串口接收缓冲区的指针
 * @note  此版本对每一次调用 MQTT_Send_Reply 都进行了返回值检查，
 *        并通过日志明确反馈回复指令是否成功发送给了4G模块。
 */
static void Process_MQTT_Message_Robust(const char* buffer)
{
    // 定义一个布尔变量，用于统一记录回复指令的发送结果
    bool reply_sent_successfully = false;
    // 打印收到的原始消息，这是调试的第一步
    printf("RECV: %s\r\n", buffer);

    // 尝试从消息中解析出 "id"，这是所有回复的凭证
    char request_id[32] = {0};
    if (!find_and_parse_json_string(buffer, "id", request_id, sizeof(request_id)))
    {
        // 如果消息里连 "id" 字段都没有，说明它不是一条需要回复的命令，直接忽略
        printf("DEBUG: Message received, but it has no 'id' field. No reply needed.\r\n");
        return;
    }
	
	delay_ms(200); 

    

    // --- 判断是哪种命令，并处理 ---

    // 1. 是不是“属性设置”命令？ (Topic 包含 /thing/property/set)
    if (strstr(buffer, "/thing/property/set") != NULL)
    {
        printf("DEBUG: Received a 'Property Set' command.\r\n");
        int parsed_value;

        char any_property_updated = 0;

        // 尝试解析 crop_stage 参数
        if (find_and_parse_json_int(buffer, "crop_stage", &parsed_value))
        {
            g_device_status.crop_stage=parsed_value;
            printf("ACTION: Cloud set 'crop_stage' to %d\r\n", g_device_status.crop_stage);
            any_property_updated=1;
        }

        // 尝试解析 fan_power 参数
        if (find_and_parse_json_int(buffer, "fan_power", &parsed_value))
        {
            //  对接收到的值进行范围检查和限制
            if (parsed_value < MIN_POWER) {
                parsed_value = MIN_POWER;
                printf("WARN: Fan power value below minimum, clamped to %d%%\r\n", MIN_POWER);
            } else if (parsed_value > MAX_POWER) {
                parsed_value = MAX_POWER;
                printf("WARN: Fan power value above maximum, clamped to %d%%\r\n", MAX_POWER);
            }

            // 执行命令：更新全局变量
            g_device_status.fan_power = parsed_value; 
            printf("ACTION: Cloud set 'fan_power' to %d%%\r\n", g_device_status.fan_power);
            Fan_Set_Speed(g_device_status.fan_power); // 立即应用新的风扇功率设置
            printf("ACTION: Fan speed adjusted to %d%%\r\n", g_device_status.fan_power);
            any_property_updated=1;
        }
        // 尝试解析 heater_power 参数
        if (find_and_parse_json_int(buffer, "heater_power", &parsed_value))
        {
            // 对接收到的值进行范围检查和限制
            if (parsed_value < MIN_POWER) {parsed_value = MIN_POWER;printf("WARN: Heater power value below minimum, clamped to %d%%\r\n", MIN_POWER);} 
            if (parsed_value > MAX_POWER) {parsed_value = MAX_POWER;printf("WARN: Heater power value above maximum, clamped to %d%%\r\n", MAX_POWER);}
            // 执行命令：更新全局变量
            g_device_status.heater_power = parsed_value; 
            printf("ACTION: Cloud set 'heater_power' to %d%%\r\n", g_device_status.heater_power);
            Heater_Set_Power(g_device_status.heater_power); // 立即应用新的加热器功率设置
            printf("ACTION: Heater power adjusted to %d%%\r\n", g_device_status.heater_power);
            any_property_updated=1;
        }
        // 尝试解析 sprinkler_power 参数
        if (find_and_parse_json_int(buffer, "sprinkler_power", &parsed_value))
        {
            // 对接收到的值进行范围检查和限制
            if (parsed_value < MIN_POWER) {parsed_value = MIN_POWER;printf("WARN: Sprinkler power value below minimum, clamped to %d%%\r\n", MIN_POWER);}
            if (parsed_value > MAX_POWER) {parsed_value = MAX_POWER;printf("WARN: Sprinkler power value above maximum, clamped to %d%%\r\n", MAX_POWER);}
            // 执行命令：更新全局变量
            g_device_status.sprinkler_power = parsed_value; 
            printf("ACTION: Cloud set 'sprinkler_power' to %d%%\r\n", g_device_status.sprinkler_power);
            Sprinkler_Set_Power(g_device_status.sprinkler_power); // 立即应用新的洒水器功率设置
            printf("ACTION: Sprinkler power adjusted to %d%%\r\n", g_device_status.sprinkler_power);
            any_property_updated=1;
        }
        if (find_and_parse_json_int(buffer, "pressure", &parsed_value)) 
        {
            //范围限制
            if (parsed_value<pressure_MIN) {parsed_value = pressure_MIN;printf("WARN: Pressure value below minimum, clamped to %.1f hPa\r\n", (float)pressure_MIN);}
            if (parsed_value>pressure_MAX) {parsed_value = pressure_MAX;printf("WARN: Pressure value above maximum, clamped to %.1f hPa\r\n", (float)pressure_MAX);}
            // 执行命令：更新全局变量
            g_device_status.pressure=parsed_value;
            printf("ACTION: Cloud set 'pressure' to %d%%\r\n", g_device_status.pressure);
            any_property_updated=1;
        }
        if (any_property_updated)
        {
            // 只要至少有一个参数被成功设置，就回复成功
            reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_PROPERTY_SET, NULL, 200, "Success");
        }
        else
        {
            // 如果消息中一个可识别的参数都没有，说明是无效请求
            printf("WARN: Property Set command received, but no valid parameters found.\r\n");
            reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_PROPERTY_SET, NULL, 400, "Bad Request");
        }
    }
    // 2.是不是“服务调用”命令？
    // 判断逻辑：只要同时包含 "/thing/service/" 和 "/invoke"，就认为是服务调用
    else if (strstr(buffer, "/thing/service/") != NULL && strstr(buffer, "/invoke") != NULL)
    {
        printf("DEBUG: Received a 'Service Invoke' command.\r\n");
        char method[64] = {0};

        // 尝试从 Topic 中提取 method (服务标识符)
        // 这是一个更稳健的方法，直接从Topic获取服务名
        const char *p_start = strstr(buffer, "/thing/service/");
        if (p_start != NULL) {
            p_start += strlen("/thing/service/"); // 移动指针到服务名开始的位置
            const char *p_end = strstr(p_start, "/invoke");
            if (p_end != NULL) {
                int len = p_end - p_start;
                if (len < sizeof(method)) {
                    strncpy(method, p_start, len);
                    method[len] = '\0';
                }
            }
        }

        // 判断具体是哪个服务
        if (strcmp(method, "set_intervention") == 0)
        {
            printf("DEBUG: Service is 'set_intervention'.\r\n");
            int parsed_status;

            // 尝试解析该服务需要的参数
            if (find_and_parse_json_int(buffer, "method", &parsed_status))
            {
                g_device_status.intervention_status = parsed_status; 
                printf("ACTION: Cloud invoked 'set_intervention' with status %d\r\n", g_device_status.intervention_status);
                printf("ACTION: Executing hardware control...\r\n");
                switch (g_device_status.intervention_status)
                {
                    case 0: Water_Pump_OFF(); Heater_OFF(); Fan_OFF(); break;
                    case 1: Water_Pump_ON(); Heater_OFF(); Fan_OFF(); break;
                    case 2: Water_Pump_OFF(); Heater_OFF(); Fan_ON(); break;
                    case 3: Water_Pump_OFF(); Heater_ON(); Fan_OFF(); break;
                    case 4: Water_Pump_OFF(); Heater_ON(); Fan_ON(); break;
                    default: printf("WARN: Unknown intervention status %d\r\n", g_device_status.intervention_status);
                }
                // 尝试发送“成功”的回复，并记录结果
                reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_SERVICE_INVOKE, method, 200, "Intervention status updated");
            }
            else
            {
                // 如果没找到 method 参数，这是客户端的请求错误
                printf("WARN: 'method' parameter not found for 'set_intervention' service.\r\n");
                reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_SERVICE_INVOKE, method, 400, "Bad Request");
            }
        }
        else
        {
            // 如果服务名不认识，回复404错误
            printf("WARN: Received invoke for an unknown or unparsed service: '%s'.\r\n", method);
            reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_SERVICE_INVOKE, method, 404, "Service not found");
        }
    }

    // 3.是不是“属性获取”命令？
    else if (strstr(buffer, "/thing/property/get") != NULL)
    {
        printf("DEBUG: Received a 'Property Get' command.\r\n");
        // 尝试找到 "params" 字段的位置
        char* params_start = strstr(buffer, "\"params\":");
        if (params_start != NULL)
        {
            // 找到了 "params" 字段，调用处理函数来构建回复
            reply_sent_successfully = MQTT_Reply_To_Property_Get_Refactored(request_id, params_start);
        }
        else
        {
            // 如果没找到 params 字段，这是客户端的请求错误
            printf("WARN: 'params' array not found in Property Get command.\r\n");
            // 发送“请求错误”的回复，并记录结果
            //reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_PROPERTY_GET, NULL, 400, "Bad Request");
            reply_sent_successfully = MQTT_Send_Reply(request_id, REPLY_TO_SERVICE_INVOKE, NULL, 400, "Bad Request");
        }
    }
    // 4.是不是“期望属性获取回复”消息？
    else if (strstr(buffer, "/thing/property/desired/get/reply") != NULL)
    {
        printf("DEBUG: Received a 'Desired Property Get Reply'.\r\n");
        int parsed_value;

        // 尝试从回复的 data 对象中解析 crop_stage 的值
        if (find_and_parse_json_int(buffer, "crop_stage", &parsed_value))
        {
            // 解析成功，立即更新本地状态
            g_device_status.crop_stage = parsed_value;
            printf("ACTION: Synchronized 'crop_stage' from cloud, new value is %d\r\n\r\n", g_device_status.crop_stage);
        }
        else
        {
            printf("WARN: 'crop_stage' not found in the desired property reply.\r\n\r\n");
        }
        return;
    }

    else
    {
        // 如果收到的消息包含了 "id"，但 Topic 既不是 property/set 也不是 service/invoke
        // 这可能是其他我们尚未处理的系统消息，比如 property/post/reply 等
        printf("DEBUG: Received a message with 'id' on an unhandled topic. No reply needed.\r\n");
        return; // 直接返回，不进入最后的日志打印环节
    }
    // --- 结束命令处理部分 ---
    // 在函数的最后，根据 reply_sent_successfully 的值，打印最终的执行结果日志
    if (reply_sent_successfully) 
    {
        printf("INFO: Reply for request_id '%s' was successfully sent to the 4G module.\r\n\r\n", request_id);
    } 
    else 
    {
        printf("FATAL ERROR: FAILED to send reply for request_id '%s' to the 4G module. The module did not respond with 'OK' within the timeout period. This is the likely cause of the platform timeout!\r\n\r\n", request_id);
    }
}

/**
 * @brief 上报四个温度属性
 */
static void MQTT_Publish_Only_Temperatures(float temp1, float temp2, float temp3, float temp4)
{
    // 每次调用都增加消息ID，确保与云端同步
    g_message_id++;
    // 1. 构建只包含四个温度属性的 'params' JSON 负载
    sprintf(g_json_payload, 
        "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":{"
        "\"temp1\":{\"value\":%.1f},"
        "\"temp2\":{\"value\":%.1f},"
        "\"temp3\":{\"value\":%.1f},"
        "\"temp4\":{\"value\":%.1f}"
        "}}",
        g_message_id,
        temp1, temp2, temp3, temp4
    );

    // 2. 构建 AT+QMTPUB 指令，Topic 保持不变
    //    "$sys/{product_id}/{device_name}/thing/property/post"
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);

    // 4. 等待模块处理和发送
    delay_ms(1100); // 因为报文变短了，延时可以适当缩短
}




/**
 * @brief 仅上报环境相关的四个属性 (温度、湿度、气压、风速)
 */
static void MQTT_Publish_Environment_Data(float ambient_temp, float humidity, float pressure, float wind_speed)
{
    // 每次调用都增加消息ID
    g_message_id++;
    // 1. 构建包含四个环境属性的 'params' JSON 负载
    snprintf(g_json_payload, JSON_PAYLOAD_SIZE, 
        "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":{"
        "\"ambient_temp\":{\"value\":%.1f},"
        "\"humidity\":{\"value\":%.1f},"
        "\"pressure\":{\"value\":%.1f},"
        "\"wind_speed\":{\"value\":%.1f}"
        "}}",
        g_message_id,
        ambient_temp, humidity, pressure, wind_speed
    );

    // 2. 构建 AT+QMTPUB 指令，Topic 保持不变
    //    "$sys/{product_id}/{device_name}/thing/property/post"
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
            "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);

    // 4. 等待模块处理和发送
    delay_ms(1100); // 这是一个中等长度的报文，延时1.5秒
}



/**
 * @brief 仅上报系统的人工干预状态
 * @param intervention_status 人工干预状态码
 */
static void MQTT_Publish_Intervention_Status(int intervention_status)
{
    
    // 每次调用都增加消息ID，确保每个消息的ID唯一
    g_message_id++;
    

    // 1. 构建包含 intervention_status 属性的 'params' JSON 负载
    int json_len = snprintf(g_json_payload, JSON_PAYLOAD_SIZE, 
        "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":{"
        "\"intervention_status\":{\"value\":%d}"
        "}}",
        g_message_id,
        intervention_status
    );
    
    
    //暂时注释掉
    /*
    if (json_len < 0 || json_len >= JSON_PAYLOAD_SIZE) {
        USART1_SendString("ERROR: Intervention status JSON buffer overflow!\r\n");
        return;
    }
    */

    // 2. 构建 AT+QMTPUB 指令，Topic 保持不变
    //    "$sys/{product_id}/{device_name}/thing/property/post"
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
            "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);
    

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);
    

    // 4. 等待模块处理和发送 (对于短报文，可以适当缩短延时)
    delay_ms(1100); 
}





/**
 * @brief 上报三个系统的可用性
 * @param sprinklers_available 喷淋系统是否可用
 * @param fans_available 风机系统是否可用
 * @param heaters_available 加热系统是否可用
 */
static void MQTT_Publish_Devices_Availability(int sprinklers_available, int fans_available, int heaters_available)
{
    // 每次调用都增加消息ID
    g_message_id++;

    // 1. 构建包含三个可用性属性的 'params' JSON 负载
    //    直接使用 %d 格式化整数 0 或 1
    int json_len = snprintf(g_json_payload, JSON_PAYLOAD_SIZE, 
        "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":{"
        "\"sprinklers_available\":{\"value\":%d},"
        "\"fans_available\":{\"value\":%d},"
        "\"heaters_available\":{\"value\":%d}"
        "}}",
        g_message_id,
        sprinklers_available, fans_available, heaters_available
    );

    // [健壮性检查]
    if (json_len < 0 || json_len >= JSON_PAYLOAD_SIZE) {
        printf("ERROR: Devices availability JSON buffer overflow!\r\n");
        return;
    }

    // 打印生成的Payload供调试查看，确保格式为 {"value":1} 或 {"value":0}
    printf("DEBUG: Generated Payload: %s\r\n", g_json_payload);

    // 2. 构建 AT+QMTPUB 指令 (保持不变)
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
            "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);

    // 4. 等待模块处理和发送
    delay_ms(1100); 
}


/**
 * @brief 上报风扇、加热器和洒水器的当前功率
 * @param fan_power         当前风扇功率值
 * @param heater_power      当前加热器功率值
 * @param sprinkler_power   当前洒水器功率值
 */
 static void MQTT_Publish_Device_Powers(int fan_power, int heater_power, int sprinkler_power)
 {
     g_message_id++;
     // 构建包含三个设备功率属性的 'params' JSON 负载
     snprintf(g_json_payload, JSON_PAYLOAD_SIZE, 
         "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":{"
         "\"fan_power\":{\"value\":%d},"
         "\"heater_power\":{\"value\":%d},"
         "\"sprinkler_power\":{\"value\":%d}"
         "}}",
         g_message_id,
         fan_power,
         heater_power,
         sprinkler_power
     );
 
     // 构建 AT+QMTPUB 指令
     snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
             "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
             MQTT_PRODUCT_ID, 
             MQTT_DEVICE_NAME,
             g_json_payload);
 
     // 通过串口发送指令
     USART1_SendString(g_cmd_buffer);
     delay_ms(1100); 
 }




/**
 * @brief 统一上报所有传感器和状态数据
 * @param temp1                 监测点1温度
 * @param temp2                 监测点2温度
 * @param temp3                 监测点3温度
 * @param temp4                 监测点4温度
 * @param ambient_temp          环境温度
 * @param humidity              环境湿度
 * @param pressure              大气压
 * @param wind_speed            风速
 * @param intervention_status   人工干预状态
 * @param XXX_power             功率
 * @param sprinklers_available  喷淋系统是否可用
 * @param fans_available        风机系统是否可用
 * @param heaters_available     加热系统是否可用
 * @note  此函数按顺序调用各个独立的数据上报函数，并在每次调用后加入短暂延时，
 *        以确保模块有足够的时间处理和发送每条消息。
 */
 void MQTT_Publish_All_Data(const DeviceStatus* status)
 {
     printf("INFO: === Begin publishing all data from status struct ===\r\n");
 
     // 1. 上报环境数据
     printf("INFO: Publishing environment data...\r\n");
     // 使用 -> 操作符通过指针访问结构体成员
     MQTT_Publish_Environment_Data(status->ambient_temp, status->humidity, status->pressure, status->wind_speed);


     Handle_Serial_Reception();
 
     // 2. 上报四个监测点温度
     printf("INFO: Publishing point temperatures...\r\n");
     MQTT_Publish_Only_Temperatures(status->temp1, status->temp2, status->temp3, status->temp4);


     Handle_Serial_Reception();
 
     // 3. 上报人工干预状态
     printf("INFO: Publishing intervention status...\r\n");
     MQTT_Publish_Intervention_Status(status->intervention_status);


     Handle_Serial_Reception();
 
     // 4. 上报设备功率
     printf("INFO: Publishing device powers...\r\n");
     MQTT_Publish_Device_Powers(status->fan_power, status->heater_power, status->sprinkler_power);


     Handle_Serial_Reception();
 
     // 5. 上报设备可用性
     printf("INFO: Publishing devices availability...\r\n");
     MQTT_Publish_Devices_Availability(status->sprinklers_available, status->fans_available, status->heaters_available);

     Handle_Serial_Reception();
     printf("INFO: === Finished publishing all data ===\r\n\r\n");
 }




/**
 * @brief 处理串口接收的下行消息 (带空闲检测)
 * @note  此函数通过检测串口总线在一定时间内的“空闲”状态，来判断一条消息是否已完整接收。
 *        它应该在主循环中被持续调用。
 */
 /*
 void Handle_Serial_Reception(void)
 {
     static unsigned int last_recv_num = 0;
     static uint64_t last_recv_time = 0;
     const uint32_t recv_idle_timeout_ms = 50; // 定义50毫秒为总线空闲超时
 
     // 检查串口接收缓冲区是否有数据
     if (xUSART.USART1ReceivedNum > 0)
     {
         // 如果当前的接收字节数 > 上次记录的字节数，说明有新数据进来
         if (xUSART.USART1ReceivedNum > last_recv_num)
         {
             // 更新“最后一次接收到数据的时间戳”
             last_recv_time = System_GetTimeMs();
             // 更新“上次记录的字节数”
             last_recv_num = xUSART.USART1ReceivedNum;
         }
         
         // 检查“当前时间”与“最后一次接收到数据的时间”之差是否超过了空闲超时阈值
         // 并且确保接收缓冲区里确实有数据 (last_recv_num > 0)
         if ((last_recv_num > 0) && (System_GetTimeMs() - last_recv_time > recv_idle_timeout_ms))
         {
             // 如果超过了50ms没有新数据进来，我们判定这是一条完整的消息
             printf("INFO: Full message received after idle period.\r\n");
             
             // --- 开始处理 ---
             // 1. 在数据末尾添加字符串结束符
             xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
             // 2. 调用统一的消息处理函数
             Process_MQTT_Message_Robust((char*)xUSART.USART1ReceivedBuffer);                
             
             // --- 处理完毕后，彻底清零所有状态，为接收下一条消息做准备 ---
             xUSART.USART1ReceivedNum = 0;
             last_recv_num = 0;
         }
     }
 }

*/



/**
 * @brief 处理串口接收的下行消息
 */
void Handle_Serial_Reception(void)
{
    // 步骤1：检查由IDLE中断服务程序设置的数据帧接收完成标志
    if (xUSART.USART1RxFrameFlag == 1)
    {
        // --- 开始处理接收到的完整数据帧 ---
        printf("INFO: Full message received (IDLE interrupt detected).\r\n");
        
        // 步骤2：在数据末尾添加字符串结束符 '\0'
        // 这对于后续使用strstr()等字符串函数至关重要
        xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
        
        // 步骤3：调用重量级的消息处理函数
        Process_MQTT_Message_Robust((char*)xUSART.USART1ReceivedBuffer);                
        
        // 步骤4：处理完毕后，清空缓冲区计数和标志位，为接收下一帧做准备
        // 必须在临界区内执行此操作，防止在清零期间被新的RXNE中断干扰
        __disable_irq();
        xUSART.USART1ReceivedNum = 0;
        xUSART.USART1RxFrameFlag = 0; // 清除标志位，等待下一次中断
        __enable_irq();
    }
}



 /**
 * @brief  从主程序接收统一的系统状态，并触发MQTT上报
 * @param  system_status: 指向包含所有数据的 SystemStatus_t 结构体的指针
 * @note   此函数负责解包 SystemStatus_t，并将数据同步到模块内部的 g_device_status。
 */
void MQTT_Publish_All_Data_Adapt(const SystemStatus_t* system_status)
{
    // [健壮性检查] 确保传入的指针有效
    if (system_status == NULL || system_status->env_data == NULL || system_status->capabilities == NULL) {
        printf("ERROR: Invalid system status pointer passed to MQTT publish function.\r\n");
        return;
    }

    // 步骤1: 同步环境传感器数据
    const EnvironmentalData_t* env = system_status->env_data;
    g_device_status.temp1 = env->temperatures[0];
    g_device_status.temp2 = env->temperatures[1];
    g_device_status.temp3 = env->temperatures[2];
    g_device_status.temp4 = env->temperatures[3];
    g_device_status.ambient_temp = env->ambient_temp;
    g_device_status.humidity = env->humidity;
    g_device_status.wind_speed = env->wind_speed;
    g_device_status.pressure = env->pressure;

    // 步骤2: 同步系统决策状态
    g_device_status.intervention_status = (int)system_status->method;
    g_device_status.fan_power = system_status->Powers->fan_power;
    g_device_status.heater_power = system_status->Powers->heater_power;
    g_device_status.sprinkler_power = system_status->Powers->sprinkler_power;
    g_device_status.crop_stage = system_status->crop_stage;

    // 步骤3: 同步设备可用性
    const SystemCapabilities_t* caps = system_status->capabilities;
    g_device_status.sprinklers_available = caps->sprinklers_available;
    g_device_status.fans_available = caps->fans_available;
    g_device_status.heaters_available = caps->heaters_available;

    // 步骤4: 调用内部的上报函数
    printf("INFO: Unified system status synchronized, starting publish process...\r\n");
    MQTT_Publish_All_Data(&g_device_status);
}


/**
 * @brief  检查MQTT连接状态，如果断开则自动重连并重新订阅所有主题
 * @return bool: true 代表当前连接正常或重连成功, false 代表重连失败
 * @note   此函数应在主循环中定期调用。
 */
bool MQTT_Check_And_Reconnect(void)
{
    // 步骤1：发送查询指令 "AT+QMTCONN?" 来获取客户端0的连接状态
    // 根据Quectel手册，对于已连接的客户端，模块会回复 "+QMTCONN: 0,3"
    // 如果未连接或状态异常，则不会包含这个特定的回复。
    if (MQTT_Send_AT_Command("AT+QMTCONN?\r\n", "+QMTCONN: 0,3", 2000))
    {
        // 成功收到了预期的“已连接”回复，说明一切正常，直接返回true
        printf("DEBUG: MQTT connection is active.\r\n");
        return true;
    }

    // 步骤2：如果上面的指令失败或超时，说明连接已断开
    printf("WARN: MQTT connection lost! Attempting to reconnect...\r\n");
    
    // 步骤3：执行完整的初始化和连接流程
    if (Robust_Initialize_And_Connect_MQTT())
    {
        printf("SUCCESS: Reconnected to MQTT server.\r\n");
        
        // 步骤4：连接成功后，必须重新订阅所有需要的主题
        printf("INFO: Resubscribing to all topics...\r\n");
        if (MQTT_Subscribe_All_Topics())
        {
            printf("SUCCESS: All topics resubscribed.\r\n");
            return true; // 整个恢复流程成功
        }
        else
        {
            printf("FATAL: Reconnected but failed to resubscribe to topics.\r\n");
            return false; // 重新订阅失败
        }
    }
    else
    {
        printf("FATAL: Failed to reconnect to MQTT server. Will try again later.\r\n");
        return false; // 重新连接失败
    }
}

