# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an STM32F103ZET6-based frost prevention system called "御霜塔" (Frost Prevention Tower) designed to protect crops from frost damage through intelligent environmental monitoring and intervention. The system uses a multi-layered approach:

- **Sensing Layer**: Multiple temperature sensors at different heights (1-4m), humidity, wind speed, and ambient temperature monitoring
- **Decision Layer**: Analyzes temperature inversion layers and determines optimal intervention methods
- **Control Layer**: Controls fans, heaters, and sprinklers based on environmental conditions and crop growth stages

## Build Commands

### Basic Build
```bash
make all          # Build complete project (creates .elf, .hex, and .bin files)
make clean        # Clean build directory
```

### Build Targets
- Build output directory: `build/`
- Main target: `main.elf`, `main.hex`, `main.bin`
- Toolchain: ARM GCC (arm-none-eabi-)

### Build Configuration
- Debug mode: Enabled (`DEBUG = 1`)
- Optimization: `-Og` (debug-friendly optimization)
- C Standard: C11
- Character encoding: GBK (for Chinese characters in TFT display)

## Architecture

### Directory Structure
```
├── USER/                   # Application code and main logic
│   ├── main.c             # Main application entry point
│   └── Frost_Detection/   # Frost detection algorithms
├── HARDWARE/              # Hardware peripheral drivers
│   ├── MQTT/              # OneNET MQTT communication
│   ├── TFT/               # Display driver and UI
│   ├── led/               # LED indicators
│   ├── key/               # Key inputs
│   ├── Relay/             # Relay control
│   ├── FAN/               # Fan and servo control
│   ├── ds18b20/           # Temperature sensors
│   ├── DHT11/             # Humidity sensor
│   └── UART_*/            # UART communication modules
├── SYSTEM/                # System-level modules
│   ├── simulation_model/  # Environmental simulation
│   ├── usart/             # Serial communication
│   ├── tim/               # Timer management
│   ├── delay/             # Delay functions
│   ├── wwdg/              # Window watchdog
│   └── iwdg/              # Independent watchdog
├── STM32F10x_FWLib/       # STM32F10x standard peripheral library
└── CORE/                  # ARM Cortex-M3 core functions
```

### Key Components

1. **Environmental Data Structure** (`Frost_Detection.h:53-59`)
   - 4 temperature sensors at different heights
   - Humidity, ambient temperature, wind speed, pressure

2. **Intervention Methods** (`Frost_Detection.h:72-78`)
   - Sprinklers: Water-based frost protection
   - Fans: Air mixing to prevent temperature inversion
   - Heaters: Direct temperature control
   - Combined strategies

3. **MQTT Communication** (`onenet_mqtt.h`)
   - OneNET platform integration
   - Real-time data reporting
   - Remote monitoring and control

4. **TFT Display** (`tft.h`, `tft_driver.h`)
   - GUI for local monitoring
   - Chinese character support (GBK encoding)
   - Real-time data visualization

### Main Control Flow (main.c:131-260)

1. **Initialization Phase**: Hardware peripherals, sensors, MQTT, display
2. **Main Loop**:
   - Read environmental data
   - Analyze temperature inversion layers
   - Determine optimal intervention method
   - Apply control strategies
   - Update display and MQTT reporting
   - Environmental simulation updates

### Key Algorithms

- **Inversion Layer Analysis**: Detects temperature inversions that contribute to frost formation
- **Intervention Power Calculation**: Dynamic power adjustment based on environmental conditions
- **Multi-sensor Data Fusion**: Combines data from multiple sensors for accurate assessment

## Hardware Configuration

### Microcontroller
- STM32F103ZET6 (Cortex-M3, 512KB Flash, 64KB RAM)
- External oscillator configuration
- Multiple UART interfaces for different sensors

### Sensors
- DS18B20: Temperature sensors (4 units at different heights)
- DHT11: Humidity and ambient temperature
- Wind speed sensor: Via UART communication
- Pressure sensor: Via ModBUS protocol

### Actuators
- Fans with PWM speed control
- Servo motor for fan direction control
- Water pumps for sprinkler system
- Heaters with power control
- LED indicators and buzzer for alerts

## Development Notes

### Character Encoding
- Project uses GBK encoding for Chinese characters in TFT display
- Compiler flag: `--exec-charset=GBK`

### Timing Considerations
- Main system tick: 300ms (TIM4)
- Environmental simulation updates
- MQTT reporting intervals

### Safety Features
- Independent and window watchdogs
- System shutdown capability
- Emergency stop functionality via hardware buttons

### Crop Growth Stages
The system adapts frost protection strategies based on crop growth stages:
- Tight cluster stage: Critical temperature -3.9°C
- Full bloom stage: Critical temperature -2.9°C
- Small fruit stage: Critical temperature -2.3°C
- Maturation stage: Variable critical temperature