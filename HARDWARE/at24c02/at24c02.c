#include "at24c02.h"
#include "delay.h" 
#include "stm32f10x.h"
#include "stdio.h"
#include "usart.h"
#include "UART_DISPLAY.h"
//初始化I2C1
static void I2C_Delay(void)
{
    delay_us(1); 
}

//SDA引脚方向控制
static void AT24C02_SDA_Set_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // 开漏输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void AT24C02_SDA_Set_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入模式
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

//I2C协议底层实现
void AT24C02_I2C_Start(void)
{
    AT24C02_SDA_Set_OUT();
	I2C_W_SDA(1);
	I2C_W_SCL(1);
    I2C_Delay();
	I2C_W_SDA(0);
    I2C_Delay();
	I2C_W_SCL(0);
    I2C_Delay();
}

void AT24C02_I2C_Stop(void)
{
    AT24C02_SDA_Set_OUT();
	I2C_W_SDA(0);
	I2C_W_SCL(1);
    I2C_Delay();
	I2C_W_SDA(1);
    I2C_Delay();
}

// 等待应答信号，返回0表示收到ACK，1表示收到NACK
uint8_t AT24C02_I2C_WaitAck(void)
{
    uint8_t AckBit;
    AT24C02_SDA_Set_IN(); // SDA设为输入，准备接收
    I2C_W_SCL(1);
    I2C_Delay();
    AckBit = I2C_R_SDA();
    I2C_W_SCL(0);
    I2C_Delay();
    AT24C02_SDA_Set_OUT(); // 恢复为输出
    return AckBit;
}

// 发送一个字节
void AT24C02_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	AT24C02_SDA_Set_OUT();
	for (i = 0; i < 8; i++)
	{
		I2C_W_SDA(!!(Byte & (0x80 >> i)));
		I2C_W_SCL(1);
		I2C_Delay();
		I2C_W_SCL(0);
		I2C_Delay();
	}
}

// 接收一个字节
uint8_t AT24C02_I2C_ReceiveByte(void)
{
    uint8_t i, Byte = 0x00;
    AT24C02_SDA_Set_IN();
    for (i = 0; i < 8; i++)
    {
        I2C_W_SCL(1);
        I2C_Delay();
        if (I2C_R_SDA())
        {
            Byte |= (0x80 >> i);
        }
        I2C_W_SCL(0);
        I2C_Delay();
    }
    AT24C02_SDA_Set_OUT();
    return Byte;
}

// 发送ACK
void AT24C02_I2C_SendAck(void)
{
    AT24C02_SDA_Set_OUT();
    I2C_W_SDA(0);
    I2C_W_SCL(1);
    I2C_Delay();
    I2C_W_SCL(0);
    I2C_Delay();
}

// 发送NACK
void AT24C02_I2C_SendNack(void)
{
    AT24C02_SDA_Set_OUT();
    I2C_W_SDA(1);
    I2C_W_SCL(1);
    I2C_Delay();
    I2C_W_SCL(0);
    I2C_Delay();
}

// 初始化
void AT24C02_Init(void)
{
    RCC_APB2PeriphClockCmd(I2C_CLK_RCC, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // SCL和SDA都设置为开漏输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	I2C_W_SCL(1);
	I2C_W_SDA(1);
}

void AT24C02_WriteByte(uint8_t Addr, uint8_t Data)
{
    AT24C02_I2C_Start();
    AT24C02_I2C_SendByte(AT24C02_DEVICE_ADDR | 0); // 发送设备地址+写命令
    AT24C02_I2C_WaitAck();
    AT24C02_I2C_SendByte(Addr); // 发送要写入的内部地址
    AT24C02_I2C_WaitAck();
    AT24C02_I2C_SendByte(Data); // 发送数据
    AT24C02_I2C_WaitAck();
    AT24C02_I2C_Stop();
    
    delay_ms(10); // 等待EEPROM内部写周期完成
}

uint8_t AT24C02_ReadByte(uint8_t Addr)
{
    uint8_t Data;
    
    AT24C02_I2C_Start();
    AT24C02_I2C_SendByte(AT24C02_DEVICE_ADDR | 0); // 写模式，先发送地址
    AT24C02_I2C_WaitAck();
    AT24C02_I2C_SendByte(Addr);
    AT24C02_I2C_WaitAck();
    
    AT24C02_I2C_Start(); // 重复起始信号
    AT24C02_I2C_SendByte(AT24C02_DEVICE_ADDR | 1); // 读模式
    AT24C02_I2C_WaitAck();
    Data = AT24C02_I2C_ReceiveByte();
    AT24C02_I2C_SendNack(); // 发送非应答信号，结束读取
    AT24C02_I2C_Stop();
    
    return Data;
}


