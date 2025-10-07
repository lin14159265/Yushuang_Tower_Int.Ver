# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an STM32F103ZET6 microcontroller-based agricultural frost prevention system. The project implements an intelligent frost detection and intervention system that monitors environmental conditions and automatically activates protective measures to prevent crop damage from frost.

## Build System

The project uses a Makefile-based build system for ARM Cortex-M3:

- **Build**: `make` - Builds main.elf, main.hex, and main.bin files
- **Clean**: `make clean` - Removes build artifacts
- **Toolchain**: Uses arm-none-eabi-gcc cross-compiler
- **Target**: STM32F103ZET6 with High Density device configuration
- **Build Directory**: `build/` - Contains object files and output binaries

## Hardware Architecture

### Core Components
- **MCU**: STM32F103ZET6 (Cortex-M3, 72MHz)
- **Memory**: 512KB Flash, 64KB RAM
- **Communication**: Multiple USART ports for sensors and control

### Sensor Integration
- **Temperature**: DS18B20 sensors at 4 different heights (1m, 2m, 3m, 4m)
- **Humidity**: DHT11 sensor
- **Wind Speed**: Custom wind sensor via ModBUS protocol
- **Pressure**: Fixed at 1013.0 hPa (could be enhanced with actual sensor)

### Hardware Control Modules
- **Fan Control**: PWM-based fan speed control with servo angle adjustment
- **Water Pump**: Relay-controlled sprinkler system
- **Heater**: Relay-controlled heating system
- **Indicators**: RGB LED status lights and buzzer alarms
- **User Interface**: Push buttons for manual control and height selection

## Software Architecture

### Directory Structure
- **CORE/**: ARM Cortex-M3 core files (CMSIS)
- **HARDWARE/**: Hardware abstraction modules for sensors and actuators
- **SYSTEM/**: System-level drivers (delay, USART, timers, watchdogs)
- **USER/**: Application logic and main firmware
- **STM32F10x_FWLib/**: STM32 Standard Peripheral Library

### Key Software Components

#### Frost Detection System (`USER/Frost_Detection/`)
- **Environmental Data Analysis**: Reads temperature gradients at multiple heights
- **Inversion Layer Detection**: Identifies temperature inversion conditions
- **Intervention Decision Logic**: Determines optimal frost prevention strategy
- **Crop-Specific Thresholds**: Adjustable critical temperatures based on crop growth stage

#### Intervention Methods
- **INTERVENTION_NONE**: No action required
- **INTERVENTION_SPRINKLERS**: Activates water pump for ice formation protection
- **INTERVENTION_FANS_ONLY**: Uses fans to mix air layers
- **INTERVENTION_HEATERS_ONLY**: Activates heating elements
- **INTERVENTION_FANS_THEN_HEATERS**: Combined approach for severe conditions

#### Communication Protocols
- **USART1**: Debug console (115200 baud)
- **USART3**: ModBUS sensor communication (9600 baud)
- **MQTT**: Network communication for remote monitoring (implementation in HARDWARE/MQTT/)

### Real-time Control
- **Interrupt Handling**: EXTI interrupts for height selection and system control
- **Timer Management**: PWM generation for fan and servo control
- **Watchdog Support**: Both independent (IWDG) and window watchdogs (WWDG)

## Development Workflow

### Adding New Sensors
1. Create hardware module in `HARDWARE/` directory
2. Add source file to Makefile `C_SOURCES` variable
3. Add include path to Makefile `C_INCLUDES` variable
4. Implement sensor interface following existing patterns

### Modifying Intervention Logic
- Main logic in `USER/main.c:95-178`
- Frost detection algorithms in `USER/Frost_Detection/Frost_Detection.c`
- Height-specific temperature analysis in `Frost_Detection.c`

### Hardware Configuration Changes
- Pin mappings in respective hardware module files
- Clock configuration in `SYSTEM/sys/`
- Peripheral initialization in `USER/main.c:68-91`

## Key Constants and Configuration

### Physical Installation Heights
- 1m, 2m, 3m, 4m sensor heights defined in `Frost_Detection.h:7-10`

### Environmental Thresholds
- Inversion gradient: 0.5°C/m (Frost_Detection.h:27)
- Wind speed thresholds: 0.5-3.0 m/s range
- Temperature safety margins: 1.0-2.0°C

### Control Parameters
- Fan power: 20-80% PWM range
- Servo angles: Calculated based on optimal intervention height
- Crop critical temperatures: -3.9°C to -2.3°C based on growth stage

## Testing and Debugging

### Debug Output
- Uses USART1 for console output
- Environmental data printed in main loop
- System status messages via RGB LED and buzzer

### Manual Control
- EXTI8: Height selection (cycles through 0-3)
- EXTI9: Data acquisition trigger
- EXTI13: System emergency stop

## Build Configuration

### Compiler Flags
- Target: `-mcpu=cortex-m3 -mthumb`
- Optimization: `-Og` (debug-friendly)
- Standard: `-std=c11`
- Character encoding: `--exec-charset=GBK`

### Linker Script
- `STM32F103ZETx_FLASH.ld`: Memory layout for STM32F103ZET6
- Supports 512KB Flash, 64KB RAM

## System Dependencies

### Required Toolchain
- arm-none-eabi-gcc
- arm-none-eabi-binutils
- Make

### External Libraries
- STM32F10x Standard Peripheral Library
- CMSIS Core for Cortex-M3