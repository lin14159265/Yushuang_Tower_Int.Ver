#include "UART_SENSOR.h"
#include <string.h>
#include "delay.h"
#include "usart.h"
#include "stdio.h"
// 485 ModBUS 传感器问询码
// 格式 [设备地址] [功能码] [起始地址] [数据长度] [CRC16 校验] 低位在前 高位在后
const unsigned char ASK_SENSOR_CMD[4][8] = {
	{0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B}, // 温湿度传感器问询码
	{0x02, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xF9}, // 二氧化碳传感器问询码
	{0x03, 0x03, 0x00, 0x06, 0x00, 0x01, 0x65, 0xE9}, // 光照度传感器问询码
    {0xFF, 0x03, 0x00, 0x00, 0x00, 0x02, 0x1E, 0x24}, // 风速传感器问询码
};
/****** 风速传感器操作变量 ******/
extern float	 wind_speed;		// 风速值
extern uint16_t wind_power;	    // 风力等级
extern char	 air_receved_flag;   // 接收状态标志
extern uint16_t air_error_count;	    // 环境参数传感器异常次数 当传感器连续异常多次，即认为该传感器有问题
extern uint16_t busy_count;	        // 用于记录等待传感器数据的时间

//传感器返回数据的有效长度 （截止到校验码）
#define HUMIDITY_data_len	7  

static void USART3_Init(uint32_t bound)
{
	// GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能USART3，GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	// USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);			// 初始化

	// USART3_RX	  GPIOB.11初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;			  // PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);				  // 初始化

	// Usart3 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		  // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 根据指定的参数初始化VIC寄存器

	// USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;										// 串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;					// 收发模式

	USART_Init(USART3, &USART_InitStructure);	   // 初始化串口3
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // 开启串口接受中断
	USART_Cmd(USART3, ENABLE);					   // 使能串口3
}

void USART3_SendString(const unsigned char *data, uint8_t len)
{
	for(uint16_t i=0; i<len; i++ )
	{
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		USART_SendData(USART3, data[i]);
	}
}

void ModBUS_Init(void)
{
	// 初始化 USART3
	USART3_Init(9600);
}

static uint16_t Recv_Buff[22] = {0};	// 接收缓存，用户存放485传感器发来的数据
static uint16_t Recv_Index = 0;			// 用户索引，用于索引接收缓存
void USART3_IRQHandler(void)
{
	int ret = 0;
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		
		Recv_Buff[Recv_Index] = USART_ReceiveData(USART3);
		
		if ( 1 == Recv_Buff[0] )
		{
			ret = Get_Humidity_Value();
		}
		if( BUSY == ret )
			Recv_Index ++;
		else
			Recv_Index = 0;
	}
	USART_ClearFlag(USART3, USART_FLAG_RXNE); // 清中断标志
}

// CRC 校验 快速查表计算的数据表
const uint16_t crctalbeabs[] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

// CRC 校验
uint16_t crc16tablefast(uint16_t *ptr, uint16_t len)
{
	// CRC寄存器
	uint16_t crc = 0xffff;
	uint16_t i;
	uint8_t ch;

	for (i = 0; i < len; i++)
	{
		ch = *ptr++;
		crc = crctalbeabs[(ch ^ crc) & 15] ^ (crc >> 4);
		crc = crctalbeabs[((ch >> 4) ^ crc) & 15] ^ (crc >> 4);
	}
	// 返回校验码
	return crc;
}

// 温湿度数据接收与计算
// 返回值： 0~正常  -1~异常  1~正常
int Get_Humidity_Value(void)
{
	uint16_t CRC_Code = 0;
	// 下面是正常接收的逻辑
	switch (Recv_Index)
	{
	case 0:
		if( Recv_Buff[Recv_Index] != 0x01)	// 温湿度传感器数据第1帧必须是 4
			air_receved_flag = FAIL; 
		else
			air_receved_flag = BUSY; 
		break;
	case 1:
		if( Recv_Buff[Recv_Index] != 0x03)	// 温湿度传感器数据第2帧必须是 3
			air_receved_flag = FAIL; 
		else
			air_receved_flag = BUSY; 
		break;
	case 2:
		if( Recv_Buff[Recv_Index] != 0x04)	// 温湿度传感器数据第3帧必须是 4
			air_receved_flag = FAIL; 
		else
			air_receved_flag = BUSY; 
		break;
	case HUMIDITY_data_len+1:		// 接收完成，进行CRC校验
		CRC_Code = crc16tablefast(Recv_Buff, HUMIDITY_data_len);
		// 如果校验码正确，则进行数据计算。否则直接舍弃数据
		if (CRC_Code == (Recv_Buff[HUMIDITY_data_len] | (Recv_Buff[HUMIDITY_data_len+1] << 8)))
		{
			// 第五位左移八位之后和第六位进行或运行得出温度值
			int16_t data1 = (Recv_Buff[5] << 8) | Recv_Buff[6];
			// 第三位左移八位之后和第四位进行或运行得出湿度值
			int16_t data2 = (Recv_Buff[3] << 8) | Recv_Buff[4];

			wind_power = data1 ;	// 计算温度值
			wind_speed = (float)data2/ 10.0;		// 计算湿度值
			air_receved_flag = FINISH;				// 更新标志位
		}
		else
		{	// 校验失败，返回错误
			air_receved_flag = FAIL; 
		}
		break;
	default:
		break;
	}
	if( (FAIL == air_receved_flag) || (FINISH == air_receved_flag) )
	{	/* 如果接收失败 或 接收完成 */ 
		Recv_Index = 0;	// 归零索引，清空缓冲区
		memset(Recv_Buff, 0, sizeof(Recv_Buff));
	}
	return air_receved_flag;
}

void Get_Wind_Data(float *speed, uint16_t *power)
{
	if( air_error_count > 5 )
        {	
            printf("error\r\n");
            
        }
        if (FREE == air_receved_flag)
        { 	// 如果传感器处于空闲状态，就发命令进行传感器问询

            delay_ms(1);
            USART3_SendString(ASK_SENSOR_CMD[0], 8);
            air_receved_flag = BUSY;
            busy_count = 0;
            delay_ms(500);	// 为保证485数据正确，最好延时一段时间
            
        }
        else if (FINISH == air_receved_flag)
        { 	// 如果传感器数据接收成功，就把数据输出在串口助手上
            
            air_receved_flag = FREE;	// 通信成功，释放接收标志位
            air_error_count = 0;		// 通信成功，消除传感器异常计数
            delay_ms(100);	// 为保证485数据正确，可以延时一段时间才开始下一次传感器的操作
        }
        else if (BUSY == air_receved_flag)
        { 	// 如果传感器传感器长期不能完成数据接收，说明该传感器异常
            delay_ms(1);	// 必须进行一段时间的等待
            busy_count++;
            if( busy_count > 30 )
            {	// 30ms 不响应，说明传感器异常
                busy_count = 0;
                air_receved_flag = FREE;	// 释放传感器，稍后再操作一次试试
                air_error_count++;		// 记录发生了一次传感器异常
                
            }
        }
        else
        { 	// 如果接收到异常数据
            air_error_count++;		// 记录发生了一次传感器异常
            delay_ms(100);	// 必须进行一段时间的等待
        }

}
