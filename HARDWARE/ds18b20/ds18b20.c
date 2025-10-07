#include "ds18b20.h"
#include "delay.h"
#include "usart.h"
 #include "bsp_usart.h"
// 用于方便迭代的端口数组
GPIO_TypeDef * DS18B20_PORT[DS18B20_COUNT]={
    DS18B20_PORT_0,
    DS18B20_PORT_1,
    DS18B20_PORT_2,
    DS18B20_PORT_3
};
// 用于方便迭代的引脚数组
const u16 DS18B20_PINS[DS18B20_COUNT] = {
    DS18B20_PIN_0,
    DS18B20_PIN_1,
    DS18B20_PIN_2,
    DS18B20_PIN_3
};


void DS18B20_SetPin_Input(u8 sensor_index)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS18B20_PINS[sensor_index];
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 带上拉电阻的输入（DS18B20需要外部上拉，但这是一个安全的默认值）
    GPIO_Init(DS18B20_PORT[sensor_index], &GPIO_InitStructure);
}

// 辅助函数：将引脚设置为输出模式
void DS18B20_SetPin_Output(u8 sensor_index)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS18B20_PINS[sensor_index];
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_PORT[sensor_index], &GPIO_InitStructure);
}
//复位DS18B20
void DS18B20_Rst(u8 sensor_index)
{
  u16 current_pin = DS18B20_PINS[sensor_index];
  GPIO_TypeDef *current_GPIO = DS18B20_PORT[sensor_index];
  DS18B20_SetPin_Output(sensor_index);   //SET PA0 OUTPUT
  DS18B20_DQ_LOW(current_GPIO,current_pin); //拉低DQ
  delay_us(750);      //拉低750us
  DS18B20_DQ_HIGH(current_GPIO,current_pin); //DQ=1
  delay_us(15);       //15US
  //DS18B20_DQ_OUT = 0; //DQ=1
}
//等待DS18B20的回应
//返回1:未检测到DS18B20的存在
//返回0:存在
u8 DS18B20_Check(u8 sensor_index)
{
  u8 retry = 0;
  u16 current_pin = DS18B20_PINS[sensor_index];
  GPIO_TypeDef *current_GPIO = DS18B20_PORT[sensor_index];

  DS18B20_SetPin_Input(sensor_index); //SET PA0 INPUT
  while (DS18B20_DQ_READ(current_GPIO,current_pin) && retry < 200)
  {
    retry++;
    delay_us(1);
  };
  if (retry >= 200)
    return 1;
  else
    retry = 0;
  while (!DS18B20_DQ_READ(current_GPIO,current_pin) && retry < 240)
  {
    retry++;
    delay_us(1);
  };
  if (retry >= 240)
    return 1;
  return 0;
}
//从DS18B20读取一个位
//返回值：1/0
u8 DS18B20_Read_Bit(u8 sensor_index) // read one bit
{
  u8 data;
  u16 current_pin = DS18B20_PINS[sensor_index];
    GPIO_TypeDef *current_GPIO = DS18B20_PORT[sensor_index];
  DS18B20_SetPin_Output(sensor_index); //SET PA0 OUTPUT
  DS18B20_DQ_LOW(current_GPIO,current_pin); //拉低DQ
  delay_us(2);
  DS18B20_DQ_HIGH(current_GPIO,current_pin); 
  DS18B20_SetPin_Input(sensor_index);

  delay_us(3);
  if (DS18B20_DQ_READ(current_GPIO,current_pin))
    data = 1;
  else
    data = 0;
  delay_us(56);
  return data;
}
//从DS18B20读取一个字节
//返回值：读到的数据
u8 DS18B20_Read_Byte(u8 sensor_index) // read one byte
{
  u8 i, j, dat;
  dat = 0;
  for (i = 1; i <= 8; i++)
  {
    j = DS18B20_Read_Bit(sensor_index);
    dat = (j << 7) | (dat >> 1);
  }
  return dat;
}
//写一个字节到DS18B20
//dat：要写入的字节
void DS18B20_Write_Byte(u8 sensor_index,u8 dat)
{
  u8 j;
  u8 testb;
  u16 current_pin = DS18B20_PINS[sensor_index];
    GPIO_TypeDef *current_GPIO = DS18B20_PORT[sensor_index];
  DS18B20_SetPin_Output(sensor_index); //SET PA0 OUTPUT;
  for (j = 1; j <= 8; j++)
  {
    testb = dat & 0x01;
    dat = dat >> 1;
    if (testb)
    {
      DS18B20_DQ_LOW(current_GPIO,current_pin); // Write 1
      delay_us(2);
      DS18B20_DQ_HIGH(current_GPIO,current_pin);
      delay_us(61);
    }
    else
    {
      DS18B20_DQ_LOW(current_GPIO,current_pin);  // Write 0
      delay_us(61);
      DS18B20_DQ_HIGH(current_GPIO,current_pin); 
      delay_us(2);
    }
  }
}
//开始温度转换
void DS18B20_Start(u8 sensor_index) // ds1820 start convert
{
  DS18B20_Rst(sensor_index);
  DS18B20_Check(sensor_index);
  DS18B20_Write_Byte(sensor_index,0xcc); // skip rom
  DS18B20_Write_Byte(sensor_index,0x44); // convert
}

// 初始化特定传感器的DS18B20的IO口 DQ 并检测DS的存在
// 返回1: 不存在
// 返回0: 存在且已配置
u8 DS18B20_Init(u8 sensor_index)
{
    // 使能GPIOC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); 
    // 初始化特定引脚为开漏输出（1-wire的初始状态）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DS18B20_PINS[sensor_index];
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_PORT[sensor_index], &GPIO_InitStructure);
    
    // 确保引脚初始为高电平（总线空闲状态）
    GPIO_SetBits(DS18B20_PORT[sensor_index],DS18B20_PINS[sensor_index]);

    DS18B20_Rst(sensor_index);
    if (DS18B20_Check(sensor_index))
    {   // 设备不存在
        return 1;
    }
    else
    {
        DS18B20_Write_Byte(sensor_index, 0xCC); // Skip ROM
        DS18B20_Write_Byte(sensor_index, 0x4E); // 写入暂存器命令
        DS18B20_Write_Byte(sensor_index, 100);  // 写入报警阈值上限 TH（100℃）
        DS18B20_Write_Byte(sensor_index, 0x00);  // 写入报警阈值下限 TL（0℃）
        DS18B20_Write_Byte(sensor_index, 0x7F);  // 设置分辨率（0x1F=9位, 0x3F=10位, 0x5F=11位, 0x7F=12位）
        
     
        DS18B20_Rst(sensor_index);
        DS18B20_Check(sensor_index);
        DS18B20_Write_Byte(sensor_index, 0xCC); // Skip ROM
        DS18B20_Write_Byte(sensor_index, 0x48); // 复制暂存器到EEPROM
        
        
        DS18B20_DQ_HIGH(DS18B20_PORT[sensor_index],DS18B20_PINS[sensor_index]); // 释放总线
        return 0;
    }
}

// 初始化所有DS18B20传感器
void DS18B20_InitAll(void)
{
    u8 i;
    for (i = 0; i < DS18B20_COUNT; i++)
    {
        if (DS18B20_Init(i)==0)
        {
            
            printf("DS18B20 sensor %d successfully initialized!\r\n", i);
            delay_ms(500);
        }
        else 
        {   
            // 处理错误，例如打印消息说明传感器i未找到
            printf("DS18B20 sensor %d not found or failed to initialize!\r\n",i);
        }
    }
}


// 从特定ds18b20获取温度值

float DS18B20_Get_Temp(u8 sensor_index)
{
    u8 temp;
    u8 TL, TH;
    int tem;
    float temperature;
    DS18B20_Start(sensor_index); // ds1820 start convert
    DS18B20_Rst(sensor_index);
    DS18B20_Check(sensor_index);
    DS18B20_Write_Byte(sensor_index,0xcc); // 发送skip ROM命令
    DS18B20_Write_Byte(sensor_index,0xbe); // convert
    TL = DS18B20_Read_Byte(sensor_index); // LSB
    TH = DS18B20_Read_Byte(sensor_index); // MSB
    if (TH > 7)
    {
        TH = ~TH;
        TL = ~TL;
        temp = 0; //温度为负
    }
    else
        temp = 1; //温度为正
    tem = TH;   //获得高八位
    tem <<= 8;
    tem += TL;                //获得底八位
    tem = tem * 625; //转换
    temperature = tem / 10000.0f;
    if(temp)
    {
        return temperature;
    }
    else
    {
        if(-temperature < -100)
        {
          return 0;
        }
        return -temperature;
    }
        
}