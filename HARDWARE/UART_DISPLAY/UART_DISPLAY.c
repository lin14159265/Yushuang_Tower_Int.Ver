#include "UART_DISPLAY.h"
#include "onenet_mqtt.h"
xUSATR_TypeDef  xUSART;         // 声明为全局变量,方便记录信息、状态
 
 
 //////////////////////////////////////////////////////////////   USART-1   //////////////////////////////////////////////////////////////
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 /******************************************************************************
  * 函  数： USART1_Init
  * 功  能： 初始化USART1的GPIO、通信参数配置、中断优先级
  *          (8位数据、无校验、1个停止位)
  * 参  数： uint32_t baudrate  通信波特率
  * 返回值： 无
  ******************************************************************************/
 void USART1_Init(uint32_t baudrate)
 {
	 GPIO_InitTypeDef  GPIO_InitStructure;
	 NVIC_InitTypeDef  NVIC_InitStructure;
	 USART_InitTypeDef USART_InitStructure;
 
	 // 时钟使能
	 RCC->APB2ENR |= RCC_APB2ENR_USART1EN;                           // 使能USART1时钟
	 RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;                             // 使能GPIOA时钟
 
	 // GPIO_TX引脚配置
	 GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽
	 GPIO_Init(GPIOA, &GPIO_InitStructure);
	 // GPIO_RX引脚配置
	 GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	 GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
	 GPIO_Init(GPIOA, &GPIO_InitStructure);
 
	 // 中断配置
	 NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	 NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
	 NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;             // 子优先级
	 NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
	 NVIC_Init(&NVIC_InitStructure);
 
	 //USART 初始化设置
	 USART_DeInit(USART1);
	 USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
	 USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
	 USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
	 USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
	 USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	 USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
	 USART_Init(USART1, &USART_InitStructure);                       // 初始化串口
 
	 USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                  // 使能接受中断
	 USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);                  // 使能空闲中断
 
	 USART_Cmd(USART1, ENABLE);                                      // 使能串口, 开始工作
 
	 USART1->SR = ~(0x00F0);                                         // 清理中断
 
	 xUSART.USART1InitFlag = 1;                                      // 标记初始化标志
	 xUSART.USART1ReceivedNum = 0;                                   // 接收字节数清零
 }
 
/******************************************************************************
  * 函  数： USART1_IRQHandler
  * 功  能： USART1的接收中断、空闲中断、发送中断
  * 参  数： 无
  * 返回值： 无
  *
 ******************************************************************************/
 static uint8_t U1TxBuffer[4096] ;    // 用于中断发送：环形缓冲区，4096个字节
//static uint16_t U1TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
//static uint16_t U1TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

static volatile uint16_t U1TxCounter = 0;
static volatile uint16_t U1TxCount   = 0;


/*
 void USART1_IRQHandler(void)
 {
	// 增加对ORE错误的处理，打破无限中断循环
    if (USART_GetITStatus(USART1, USART_IT_ORE) != RESET)
    {
        // 清除ORE标志的标准序列：先读SR，再读DR
        (void)USART1->SR;
        (void)USART1->DR;
    }



	 // 接收中断
	 if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	 {
		 // 检查缓冲区是否已满，防止溢出
		 if(xUSART.USART1ReceivedNum < U1_RX_BUF_SIZE)
		 {
			 // 直接将接收到的数据追加到全局缓冲区的末尾
			 xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum++] = USART_ReceiveData(USART1);
		 }
		 else
		 {
			 // 缓冲区满了，丢弃数据，但要读一下DR来清除中断标志
			 USART_ReceiveData(USART1);
		 }
		 // 注意：RXNE标志在读取DR后会自动清除
	 }
 
	 // 空闲中断
	 if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	 {
		 // 空闲中断的清除方法是先读SR再读DR
		 volatile uint16_t temp;
		 temp = USART1->SR;
		 temp = USART1->DR;
	 }
 
	 // 发送中断
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
	{
		// 检查是否还有数据需要发送
		if (U1TxCounter < U1TxCount)
		{
			// 使用取模运算实现环形读取
			USART1->DR = U1TxBuffer[U1TxCounter % 4096];
			U1TxCounter++; // 已发送计数器增加
		}
		
		// 如果所有数据都已发送完毕
		if (U1TxCounter == U1TxCount)
		{
			// 关闭发送缓冲区空中断，否则会一直进入中断
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}
 }
 */

void USART1_IRQHandler(void)
{
    // 增加对ORE错误的处理，打破无限中断循环
    if (USART_GetITStatus(USART1, USART_IT_ORE) != RESET)
    {
        // 清除ORE标志的标准序列：先读SR，再读DR
        (void)USART1->SR;
        (void)USART1->DR;
    }

    // ... (接收中断和空闲中断部分保持不变) ...

    // 接收中断
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        // 检查缓冲区是否已满，防止溢出
        if(xUSART.USART1ReceivedNum != U1_RX_BUF_SIZE)
        {
            // 直接将接收到的数据追加到全局缓冲区的末尾
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum++] = USART_ReceiveData(USART1);
        }
        else
        {
            // 缓冲区满了，丢弃数据，但要读一下DR来清除中断标志
            USART_ReceiveData(USART1);
        }
        // 注意：RXNE标志在读取DR后会自动清除
    }

    // 空闲中断
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
        // 检查是否确实收到了数据
        if (xUSART.USART1ReceivedNum > 0)
        {
            // 不再调用任何处理函数，只设置标志位！
            xUSART.USART1RxFrameFlag = 1;
        }

		// 严格按照手册顺序清除IDLE标志位
		(void)USART1->SR;
		(void)USART1->DR;
	}

    // 发送中断
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        // 检查是否还有数据需要发送
        // 关键修改：将 U1TxCounter < U1TxCount 修改为 U1TxCounter != U1TxCount
        // 这样即使计数器溢出，只要两者不相等，就说明还有数据待发送。
        if (U1TxCounter != U1TxCount)
        {
            // 使用取模运算实现环形读取
            USART1->DR = U1TxBuffer[U1TxCounter % 4096];
            U1TxCounter++; // 已发送计数器增加
        }
        
        // 如果所有数据都已发送完毕
        if (U1TxCounter == U1TxCount)
        {
            // 关闭发送缓冲区空中断，否则会一直进入中断
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
        }
    }
}



 /******************************************************************************
  * 函  数： USART1_GetBuffer
  * 功  能： 获取UART所接收到的数据
  * 参  数： uint8_t* buffer   数据存放缓存地址
  *          uint8_t* cnt      接收到的字节数
  * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
  ******************************************************************************/
 uint8_t USART1_GetBuffer(uint8_t *buffer, uint8_t *cnt)
 {
	 if (xUSART.USART1ReceivedNum > 0)                                           // 判断是否有新数据
	 {
		 memcpy(buffer, xUSART.USART1ReceivedBuffer, xUSART.USART1ReceivedNum);  // 把新数据复制到指定位置
		 *cnt = xUSART.USART1ReceivedNum;                                        // 把新数据的字节数，存放指定变量
		 xUSART.USART1ReceivedNum = 0;                                           // 接收标记置0
		 return *cnt;                                                            // 返回所接收到新数据的字节数
	 }
	 return 0;                                                                   // 返回0, 表示没有接收到新数据
 }
 
 /******************************************************************************
  * 函  数： USART1_SendData
  * 功  能： UART通过中断发送数据,适合各种数据类型
  *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
  *         【不 适 合】注意环形缓冲区容量4096字节，如果发送频率太高，注意波特率
  * 参  数： uint8_t* buf   需发送数据的首地址
  *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于4096个字节
  * 返回值：
  ******************************************************************************/
/*
void USART1_SendData(uint8_t *buf, uint16_t cnt) 
{
	// 在操作共享的环形缓冲区时，关闭中断以防止竞争条件
	__disable_irq();

	for (uint8_t i = 0; i < cnt; i++)
    {
		// 使用取模运算实现环形写入
		U1TxBuffer[U1TxCount % 4096] = buf[i];
		U1TxCount++;
    }

	// 使能发送中断
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

	// 恢复中断
	__enable_irq();
}
*/
void USART1_SendData(uint8_t *buf, uint16_t cnt) 
{
    uint16_t i;
    // 检查缓冲区剩余空间
    for (i = 0; i < cnt; i++)
    {
        // 等待直到缓冲区有空间
        // (U1TxCount - U1TxCounter) 是当前缓冲区中的数据量
        while ((uint16_t)(U1TxCount - U1TxCounter) >= 4096)
        {
            // 缓冲区已满，等待消费者消费数据
            // 这里可以加一个超时退出机制，防止死锁
        }

        // 在操作共享的环形缓冲区时，关闭中断以防止竞争条件
        __disable_irq();

        // 使用取模运算实现环形写入
        U1TxBuffer[U1TxCount % 4096] = buf[i];
        U1TxCount++;

        // 恢复中断
        __enable_irq();
    }

    // 使能发送中断 (确保中断在有数据时总是开启的)
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}
 
 /******************************************************************************
  * 函  数： USART1_SendString
  * 功  能： UART通过中断发送输出字符串,无需输入数据长度
  *         【适合场景】字符串，长度<=4096字节
  *         【不 适 合】int,float等数据类型
  * 参  数： char* stringTemp   需发送数据的缓存首地址
  * 返回值：
  ******************************************************************************/
void USART1_SendString(char *stringTemp)
{
	uint16_t num = 0;
	while (*(stringTemp + num) != '\0' && num < 4096)
	{
		num++;
	}
	
	if (num > 0)
	{
		USART1_SendData((uint8_t *)stringTemp, num);
	}
}
 
 /******************************************************************************
  * 函  数： USART1_SendStringForDMA
  * 功  能： UART通过DMA发送数据，省了占用中断的时间
  *         【适合场景】字符串，字节数非常多
  *         【不 适 合】1:只适合发送字符串，不适合发送可能含0的数值类数据; 2-时间间隔要足够
  * 参  数： char* stringTemp  要发送的字符串首地址
  * 返回值： 无
  ******************************************************************************/
 void USART1_SendStringForDMA(char *stringTemp)
 {
	 static u8 Flag_DmaTxInit = 0;                // 用于标记是否已配置DMA发送
	 u32   num = 0;                               // 发送的数量
	 char *t = stringTemp ;                       // 用于配合计算发送的数量
 
	 while (*t++ != 0)  num++;                    // 计算要发送的数目
 
	 while (DMA1_Channel4->CNDTR > 0);            // 重要：如果DMA还在进行上次发送，就等待
	 if (Flag_DmaTxInit == 0)                     // 是否已进行过USAART_TX的DMA传输配置
	 {
		 Flag_DmaTxInit  = 1;                     // 设置标记，下次调用本函数就不再进行配置了
		 USART1 ->CR3   |= 1 << 7;                // 使能DMA发送
		 RCC->AHBENR    |= 1 << 0;                // 开启DMA1时钟
 
		 DMA1_Channel4->CCR   = 0;                // 失能， 清0整个寄存器, DMA必须失能才能配置
		 DMA1_Channel4->CNDTR = num;              // 传输数据量
		 DMA1_Channel4->CMAR  = (u32)stringTemp;  // 存储器地址
		 DMA1_Channel4->CPAR  = (u32)&USART1->DR; // 外设地址
 
		 DMA1_Channel4->CCR |= 1 << 4;            // 数据传输方向   0:从外设读   1:从存储器读
		 DMA1_Channel4->CCR |= 0 << 5;            // 循环模式       0:不循环     1：循环
		 DMA1_Channel4->CCR |= 0 << 6;            // 外设地址非增量模式
		 DMA1_Channel4->CCR |= 1 << 7;            // 存储器增量模式
		 DMA1_Channel4->CCR |= 0 << 8;            // 外设数据宽度为8位
		 DMA1_Channel4->CCR |= 0 << 10;           // 存储器数据宽度8位
		 DMA1_Channel4->CCR |= 0 << 12;           // 中等优先级
		 DMA1_Channel4->CCR |= 0 << 14;           // 非存储器到存储器模式
	 }
	 DMA1_Channel4->CCR  &= ~((u32)(1 << 0));     // 失能，DMA必须失能才能配置
	 DMA1_Channel4->CNDTR = num;                  // 传输数据量
	 DMA1_Channel4->CMAR  = (u32)stringTemp;      // 存储器地址
	 DMA1_Channel4->CCR  |= 1 << 0;               // 开启DMA传输
 }




 void USART2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 时钟使能
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;                           // 使能USART2时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;                             // 使能GPIOA时钟

    // GPIO_TX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // GPIO_RX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // 中断配置
    NVIC_InitStructure .NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
    NVIC_InitStructure .NVIC_IRQChannelSubPriority = 2;             // 子优先级
    NVIC_InitStructure .NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    //USART 初始化设置
    //USART_DeInit(USART2);
    USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
    USART_Init(USART2, &USART_InitStructure);                       // 初始化串口

    USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);                  // 使能接受中断
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);                  // 使能空闲中断

    USART_Cmd(USART2, ENABLE);                                      // 使能串口, 开始工作

    USART2->SR = ~(0x00F0);                                         // 清理中断

    xUSART.USART2InitFlag = 1;                                      // 标记初始化标志
    xUSART.USART2ReceivedNum = 0;                                   // 接收字节数清零

    printf("\rUSART2初始化配置      接收中断、空闲中断, 发送中断\r");
}

/******************************************************************************
 * 函  数： USART2_IRQHandler
 * 功  能： USART2的接收中断、空闲中断、发送中断
 * 参  数： 无
 * 返回值： 无
 ******************************************************************************/
static uint8_t U2TxBuffer[256] ;    // 用于中断发送：环形缓冲区，256个字节
static uint8_t U2TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
static uint8_t U2TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

void USART2_IRQHandler(void)
{
    static uint16_t cnt = 0;                                         // 接收字节数累计：每一帧数据已接收到的字节数
    static uint8_t  RxTemp[U2_RX_BUF_SIZE];                          // 接收数据缓存数组：每新接收１个字节，先顺序存放到这里，当一帧接收完(发生空闲中断), 再转存到全局变量：xUSART.USARTxReceivedBuffer[xx]中；

    // 接收中断
    if (USART2->SR & (1 << 5))                                       // 检查RXNE(读数据寄存器非空标志位); RXNE中断清理方法：读DR时自动清理；
    {
        if ((cnt >= U2_RX_BUF_SIZE))//||xUSART.USART2ReceivedFlag==1 // 判断1: 当前帧已接收到的数据量，已满(缓存区), 为避免溢出，本包后面接收到的数据直接舍弃．
        {
            // 判断2: 如果之前接收好的数据包还没处理，就放弃新数据，即，新数据帧不能覆盖旧数据帧，直至旧数据帧被处理．缺点：数据传输过快于处理速度时会掉包；好处：机制清晰，易于调试
            USART2->DR;                                              // 读取数据寄存器的数据，但不保存．主要作用：读DR时自动清理接收中断标志；
            return;
        }
        RxTemp[cnt++] = USART2->DR ;                                 // 把新收到的字节数据，顺序存放到RXTemp数组中；注意：读取DR时自动清零中断位；
    }

    // 空闲中断, 用于配合接收中断，以判断一帧数据的接收完成
    if (USART2->SR & (1 << 4))                                       // 检查IDLE(空闲中断标志位); IDLE中断标志清理方法：序列清零，USART1 ->SR;  USART1 ->DR;
    {
        xUSART.USART2ReceivedNum  = 0;                               // 把接收到的数据字节数清0
        memcpy(xUSART.USART2ReceivedBuffer, RxTemp, U2_RX_BUF_SIZE); // 把本帧接收到的数据，存放到全局变量xUSART.USARTxReceivedBuffer中, 等待处理; 注意：复制的是整个数组，包括0值，以方便字符串数据
        xUSART.USART2ReceivedNum  = cnt;                             // 把接收到的字节数，存放到全局变量xUSART.USARTxReceivedCNT中；
        cnt = 0;                                                     // 接收字节数累计器，清零; 准备下一次的接收
        memset(RxTemp, 0, U2_RX_BUF_SIZE);                           // 接收数据缓存数组，清零; 准备下一次的接收
        USART2 ->SR;
        USART2 ->DR;                                 // 清零IDLE中断标志位!! 序列清零，顺序不能错!!
    }

    // 发送中断
    if ((USART2->SR & 1 << 7) && (USART2->CR1 & 1 << 7))             // 检查TXE(发送数据寄存器空)、TXEIE(发送缓冲区空中断使能)
    {
        USART2->DR = U2TxBuffer[U2TxCounter++];                      // 读取数据寄存器值；注意：读取DR时自动清零中断位；
        if (U2TxCounter == U2TxCount)
            USART2->CR1 &= ~(1 << 7);                                // 已发送完成，关闭发送缓冲区空置中断 TXEIE
    }
}

/******************************************************************************
 * 函  数： vUSART2_GetBuffer
 * 功  能： 获取UART所接收到的数据
 * 参  数： uint8_t* buffer   数据存放缓存地址
 *          uint8_t* cnt      接收到的字节数
 * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
 ******************************************************************************/
uint8_t USART2_GetBuffer(uint8_t *buffer, uint8_t *cnt)
{
    if (xUSART.USART2ReceivedNum > 0)                                          // 判断是否有新数据
    {
        memcpy(buffer, xUSART.USART2ReceivedBuffer, xUSART.USART2ReceivedNum); // 把新数据复制到指定位置
        *cnt = xUSART.USART2ReceivedNum;                                       // 把新数据的字节数，存放指定变量
        xUSART.USART2ReceivedNum = 0;                                          // 接收标记置0
        return *cnt;                                                           // 返回所接收到新数据的字节数
    }
    return 0;                                                                  // 返回0, 表示没有接收到新数据
}

/******************************************************************************
 * 函  数： vUSART2_SendData
 * 功  能： UART通过中断发送数据,适合各种数据类型
 *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
 *         【不 适 合】注意环形缓冲区容量256字节，如果发送频率太高，注意波特率
 * 参  数： uint8_t* buffer   需发送数据的首地址
 *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于256个字节
 * 返回值：
 ******************************************************************************/
void USART2_SendData(uint8_t *buf, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
        U2TxBuffer[U2TxCount++] = buf[i];

    if ((USART2->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
        USART2->CR1 |= 1 << 7;
}

/******************************************************************************
 * 函  数： vUSART2_SendString
 * 功  能： UART通过中断发送输出字符串,无需输入数据长度
 *         【适合场景】字符串，长度<=256字节
 *         【不 适 合】int,float等数据类型
 * 参  数： char* stringTemp   需发送数据的缓存首地址
 * 返回值： 元
 ******************************************************************************/
void USART2_SendString(char *stringTemp)
{
    u16 num = 0;                                 // 字符串长度
    char *t = stringTemp ;                       // 用于配合计算发送的数量
    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位
    USART2_SendData((u8 *)stringTemp, num);      // 注意调用函数所需要的真实数据长度; 如果目标需要以0作结尾判断，需num+1：字符串以0结尾，即多发一个:0
}