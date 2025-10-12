# 御霜塔智能防霜系统 (Yushuang Tower Frost Prevention System)

![Version](https://img.shields.io/badge/version-2.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-STM32F103ZET6-green.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

基于STM32F103ZET6的智能农业防霜系统，通过多层环境监测和智能决策保护农作物免受霜冻危害。

## 🌟 项目特性

- **多层环境感知**: 1-4米高度的温度梯度监测、湿度、风速和气压检测
- **智能决策系统**: 基于逆温层分析和作物生长阶段的精准干预策略
- **多种保护方式**: 风机、加热器、喷淋系统的协同控制
- **远程监控**: 基于OneNET平台的MQTT实时数据上报
- **本地可视化**: TFT触摸屏显示界面，支持中文字符
- **高可靠性**: 双看门狗保护、自动重连机制、紧急停止功能

## 🏗️ 系统架构

```
御霜塔防霜系统
├── 感知层 (Sensing Layer)
│   ├── DS18B20温度传感器 (4个，1-4米高度)
│   ├── DHT11温湿度传感器
│   ├── 风速传感器 (ModBUS)
│   └── 气压传感器 (ModBUS)
├── 决策层 (Decision Layer)
│   ├── 逆温层分析算法
│   ├── 作物生长阶段适配
│   └── 多传感器数据融合
├── 控制层 (Control Layer)
│   ├── 风机系统 (PWM调速 + 舵机方向控制)
│   ├── 加热系统 (功率可调)
│   ├── 喷淋系统 (水泵控制)
│   └── 报警系统 (LED + 蜂鸣器)
└── 通信层 (Communication Layer)
    ├── MQTT通信 (OneNET平台)
    ├── 串口通信 (ModBUS协议)
    └── 本地显示 (TFT触摸屏)
```

## 🚀 快速开始

### 环境要求

- **工具链**: ARM GCC (arm-none-eabi-)
- **开发环境**: Keil MDK / STM32CubeIDE / VSCode + PlatformIO
- **硬件**: STM32F103ZET6开发板

### 编译与烧录

```bash
# 克隆项目
git clone <repository-url>
cd Yushuang_Tower_Int.Ver

# 编译项目
make all          # 完整编译 (生成 .elf, .hex, .bin 文件)
make clean        # 清理编译目录

# 烧录到STM32 (使用ST-Link)
st-flash write build/main.bin 0x08000000
```

### 编译配置

- **调试模式**: 启用 (`DEBUG = 1`)
- **优化级别**: `-Og` (调试友好优化)
- **C标准**: C11
- **字符编码**: GBK (支持TFT中文字符显示)

## 📁 项目结构

```
├── USER/                   # 应用代码和主逻辑
│   ├── main.c             # 主程序入口
│   └── Frost_Detection/   # 霜冻检测算法
├── HARDWARE/              # 硬件外设驱动
│   ├── MQTT/              # OneNET MQTT通信
│   ├── TFT/               # 显示驱动和UI
│   ├── led/               # LED指示灯
│   ├── key/               # 按键输入
│   ├── Relay/             # 继电器控制
│   ├── FAN/               # 风机和舵机控制
│   ├── ds18b20/           # 温度传感器
│   ├── DHT11/             # 湿度传感器
│   └── UART_*/            # UART通信模块
├── SYSTEM/                # 系统级模块
│   ├── simulation_model/  # 环境仿真
│   ├── usart/             # 串口通信
│   ├── tim/               # 定时器管理
│   ├── delay/             # 延时函数
│   ├── wwdg/              # 窗口看门狗
│   └── iwdg/              # 独立看门狗
├── STM32F10x_FWLib/       # STM32F10x标准外设库
└── CORE/                  # ARM Cortex-M3核心函数
```

## 🔧 核心算法

### 逆温层分析

系统通过分析不同高度的温度梯度，检测导致霜冻形成的逆温层现象：

```c
// 温度梯度计算
float temp_gradient[3];
for(int i = 0; i < 3; i++) {
    temp_gradient[i] = temps[i+1] - temps[i];
}

// 逆温层判定
if(temp_gradient[0] > 0 && temp_gradient[1] > 0) {
    // 检测到逆温层，启动风机干预
    activate_fan_intervention();
}
```

### 干预功率计算

根据环境条件动态调整干预设备功率：

```c
// 功率 = 基础功率 × 温差系数 × 湿度系数 × 风速系数
float intervention_power = base_power *
                          temp_factor *
                          humidity_factor *
                          wind_speed_factor;
```

### MQTT状态机

分块数据传输，确保通信可靠性：

```c
typedef enum {
    MQTT_STATE_INIT,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_SENDING_HEADER,
    MQTT_STATE_SENDING_DATA,
    MQTT_STATE_SENDING_FOOTER,
    MQTT_STATE_WAITING_ACK,
    MQTT_STATE_DISCONNECTED
} mqtt_state_t;
```

## 🌡️ 硬件配置

### 主要参数

| 组件 | 型号 | 参数 | 连接方式 |
|------|------|------|----------|
| 微控制器 | STM32F103ZET6 | Cortex-M3, 512KB Flash, 64KB RAM | - |
| 温度传感器 | DS18B20 | -55°C ~ +125°C, ±0.5°C | OneWire |
| 温湿度传感器 | DHT11 | 0-50°C, 20-90%RH | 单总线 |
| 显示屏 | TFT LCD | 320x240, SPI接口 | SPI |
| 通信模块 | ESP8266/ENC28J60 | WiFi/以太网 | UART/SPI |

### 引脚分配

```c
// UART配置
USART1: 115200, PA9(TX), PA10(RX)  // 调试串口
USART2: 115200, PA2(TX), PA3(RX)   // WiFi模块
USART3: 9600,  PB10(TX), PB11(RX)  // ModBUS传感器

// 传感器引脚
DS18B20: PA0  // 温度传感器
DHT11:   PA1  // 温湿度传感器

// 控制输出
FAN_PWM:   PA6  // 风机PWM控制
SERVO:     PA7  // 舵机控制
HEATER:    PB0  // 加热器
PUMP:      PB1  // 水泵
LED:       PC13 // 状态指示LED
```

## ⏱️ 时序配置

| 功能 | 周期 | 说明 |
|------|------|------|
| 系统时钟 | 1ms | SysTick系统滴答 |
| 传感器读取 | 2000ms | 温湿度数据采集 |
| MQTT数据上报 | 1200ms | 分块数据传输 |
| MQTT心跳 | 60000ms | 连接保活 |
| 看门狗喂狗 | 1600ms | 系统超时保护 |
| 环境仿真 | 5000ms | 模拟数据更新 |

## 🌱 作物保护策略

系统根据不同作物生长阶段调整防霜策略：

| 生长阶段 | 临界温度 | 保护策略 |
|----------|----------|----------|
| 花簇紧实期 | -3.9°C | 风机 + 加热器 |
| 盛花期 | -2.9°C | 风机 + 喷淋 |
| 幼果期 | -2.3°C | 综合干预 |
| 成熟期 | 可变 | 动态调整 |

## 🛡️ 安全特性

- **双重看门狗**: 独立看门狗(IWDG) + 窗口看门狗(WWDG)
- **系统关闭**: 紧急情况下可安全关闭所有设备
- **急停功能**: 硬件按钮触发的紧急停止
- **通信保护**: MQTT自动重连机制
- **数据备份**: 关键参数EEPROM存储

## 📊 监控界面

### 本地TFT显示

```
┌─────────────────────────┐
│   御霜塔防霜系统 v2.0   │
├─────────────────────────┤
│ 温度: 15.2°C  湿度: 65% │
│ 风速: 2.3m/s  气压: 1013│
├─────────────────────────┤
│ 1m: 16.1°C  2m: 15.8°C  │
│ 3m: 15.2°C  4m: 14.9°C  │
├─────────────────────────┤
│ 状态: 正常运行          │
│ 风机: 停止  加热: 停止  │
│ 喷淋: 停止  MQTT: 连接  │
└─────────────────────────┘
```

### OneNET远程监控

- 实时数据可视化
- 历史数据查询
- 远程控制接口
- 报警信息推送

## 🔍 故障排除

### 常见问题

1. **MQTT连接失败**
   - 检查WiFi模块供电
   - 验证OneNET平台配置
   - 查看串口调试信息

2. **传感器读取异常**
   - 检查OneWire总线连接
   - 验证传感器供电
   - 更换故障传感器

3. **系统重启频繁**
   - 检查看门狗配置
   - 确认电源稳定性
   - 查看内存使用情况

### 调试信息

```bash
# 串口调试 (115200 8N1)
UART输出示例:
[INFO] 系统初始化完成
[INFO] MQTT连接成功
[WARN] 检测到温度逆温层
[INFO] 启动风机干预
```

## 📈 开发路线图

- [ ] **v2.1**: 机器学习算法优化
- [ ] **v2.2**: 多设备协同控制
- [ ] **v2.3**: LoRa无线通信扩展
- [ ] **v2.4**: 太阳能供电系统
- [ ] **v3.0**: 边缘计算AI芯片集成

## 🤝 贡献指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📝 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情

## 👥 作者与致谢

- **主要开发者**: [Your Name]
- **硬件设计**: [Hardware Team]
- **算法支持**: [Algorithm Team]

---

**御霜塔智能防霜系统** - 守护农作物，保障丰收 🌾

如有问题或建议，请提交 [Issue](../../issues) 或联系开发团队。
