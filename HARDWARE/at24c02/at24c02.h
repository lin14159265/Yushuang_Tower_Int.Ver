#ifndef __AT24C02_H
#define __AT24C02_H

#include "sys.h" 
#include <stdint.h>

#define I2C_PORT      GPIOC
#define I2C_SCL_PIN   GPIO_Pin_4
#define I2C_SDA_PIN   GPIO_Pin_5
#define I2C_CLK_RCC   RCC_APB2Periph_GPIOC

// 底层引脚操作宏
#define I2C_W_SCL(x)  GPIO_WriteBit(I2C_PORT, I2C_SCL_PIN, (BitAction)(x))
#define I2C_W_SDA(x)  GPIO_WriteBit(I2C_PORT, I2C_SDA_PIN, (BitAction)(x))
#define I2C_R_SDA()   GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN)

void AT24C02_Init(void);
void AT24C02_WriteByte(uint8_t Addr, uint8_t Data);
uint8_t AT24C02_ReadByte(uint8_t Addr);

#define AT24C02_DEVICE_ADDR        0xA0 // AT24C02的设备地址
#define EEPROM_ADDR_SAFETY_MARGIN  0x10 // 用地址 0x10 存储安全边际
#define EEPROM_ADDR_CROP_STAGE     0x11 // 用地址 0x11 存储作物物候期
#define EEPROM_ADDR_NUMBER         0x00 // 用地址 0x00 判断是否是第一次上电

#define EEPROM_VALUE               0x5A // 用地址 0x5A 判断是否完成初始化

#endif
