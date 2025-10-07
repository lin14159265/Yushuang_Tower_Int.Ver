#include "UART_DISPLAY.h"
#include <string.h> 

/*
 ===============================================================================
                            模块内部变量定义
 ===============================================================================
*/
// 定义串口1的接收缓冲区
uint8_t g_usart1_rx_buffer[USART1_RX_BUFFER_SIZE];
// 定义接收完成标志位 (0: 未完成, 1: 已接收到一条完整消息)
// volatile 关键字防止编译器进行不当优化，确保在主循环和中断中都能正确访问
volatile uint8_t g_usart1_rx_flag = 0;

// 定义一个静态变量，用于在中断中记录当前接收到的数据长度
static volatile uint16_t g_usart1_rx_len = 0;


/*
 ===============================================================================
                            函数实现
 ===============================================================================
*/

void USART1_Init(uint32_t bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	/* USART1 GPIO config */
	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* USART1 mode config */
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	/* 
	 * =====================================================
	 *        重点：重新使能 NVIC 和 串口接收中断
	 * =====================================================
	 */
	// USART1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		  // 串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		  // 子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 根据指定的参数初始化VIC寄存器
	
    // 开启接收中断 (当有数据到达时，触发中断)
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); 
    
    // 使能串口
	USART_Cmd(USART1, ENABLE);
}

void USART1_SendData(uint8_t data)
{
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, data);
}

void USART1_SendString(uint8_t *data, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
	{
		USART1_SendData(data[i]);
	}
}

/**
 * @brief  清空接收缓冲区和相关标志位
 * @note   在主程序处理完一条消息后，应调用此函数
 */
void Clear_USART1_Buffer(void)
{
    // 使用 memset 快速将缓冲区清零
    memset(g_usart1_rx_buffer, 0, USART1_RX_BUFFER_SIZE);
    g_usart1_rx_len = 0;
    g_usart1_rx_flag = 0;
}


/**
 * @brief  USART1 中断服务函数
 * @note   此函数是中断处理的核心。当串口接收到数据时，硬件会自动调用此函数。
 */
void USART1_IRQHandler(void)
{
    uint8_t received_data;

    // 检查是否是接收中断 (接收缓冲区非空)
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        // 读取接收到的数据，此操作会自动清除 USART_IT_RXNE 标志位
        received_data = USART_ReceiveData(USART1);

        // 防止缓冲区溢出
        if (g_usart1_rx_len < (USART1_RX_BUFFER_SIZE - 1))
        {
            // 将接收到的数据存入缓冲区
            g_usart1_rx_buffer[g_usart1_rx_len++] = received_data;
        }

        // 判断是否接收到一条完整指令 (AT指令通常以 "\r\n" 结尾)
        if (received_data == '\n')
        {
            g_usart1_rx_buffer[g_usart1_rx_len] = '\0'; // 添加字符串结束符
            g_usart1_rx_flag = 1; // 设置接收完成标志
        }
    }
}