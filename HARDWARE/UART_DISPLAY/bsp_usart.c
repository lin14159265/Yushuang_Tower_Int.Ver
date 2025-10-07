/***********************************************************************************************************************************
 **【文件名称】  bsp_usart.c
 **
 **【文件功能】  各USART的GPIO配置、通信协议配置、中断配置，及功能函数实现
 **
 **【适用平台】  STM32F103 + 标准库v3.5 + keil5
 **
 ** 【代码说明】  本文件的收发机制, 经多次修改, 已比较完善.
 **               初始化: 只需调用：USARTx_Init(波特率), 函数内已做好引脚及时钟配置；
 **               发 送 : 两个函数, 字符串: USARTx_SendString (char* stringTemp)、 数据: USARTx_SendData (uint8_t* buf, uint8_t cnt);
 **               接 收 : 方式1：通过全局函数USARTx_GetBuffer (uint8_t* buffer, uint8_t* cnt);　本函数已清晰地示例了接收机制；
 **                       方式2：通过判断: xUSART.USART1ReceivedNum > 0;
 **                              如在while中不断轮询，或在任何一个需要的地方判断接收到的字节数是否大于0．示例：
 **                              while(1){
 **                                  if(xUSART.USART1ReceivedNum>0){
 **                                      printf((char*)xUSART.USART1ReceivedBuffer);          // 示例1: 如何输出成字符串
 **                                      uint16_t GPS_Value = xUSART.USART1ReceivedBuffer[1]; // 示例2: 如何读写其中某个成员的数据
 **                                      xUSART.USART1ReceivedNum = 0;                     // 重要：处理完数据后
 **                                  }
 **                              }
 ************************************************************************************************************************************/
 #include "bsp_usart.h"
 #include "stm32f10x.h"
 
 
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
 static uint16_t U1TxBuffer[4096] ;    // 用于中断发送：环形缓冲区，4096个字节
 static uint16_t U1TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
 static uint16_t U1TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)
 
 void USART1_IRQHandler(void)
 {
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
	 if ((USART1->SR & 1 << 7) && (USART1->CR1 & 1 << 7))
	 {
		 USART1->DR = U1TxBuffer[U1TxCounter++];
		 if (U1TxCounter == U1TxCount)
			 USART1->CR1 &= ~(1 << 7);
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
 void USART1_SendData(uint8_t *buf, uint8_t cnt)
 {
	 for (uint16_t i = 0; i < cnt; i++)
		 U1TxBuffer[U1TxCount++] = buf[i];
 
	 if ((USART1->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
		 USART1->CR1 |= 1 << 7;
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
	 u16 num = 0;                                 // 字符串长度
	 char *t = stringTemp ;                       // 用于配合计算发送的数量
	 while (*t++ != 0)  num++;                    // 计算要发送的数目
	 USART1_SendData((u8 *)stringTemp, num);      // 调用函数完成发送
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

 