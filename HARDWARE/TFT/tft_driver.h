#ifndef _TFT_DRIVER_H
#define _TFT_DRIVER_H

#include "sys.h"
#include "Frost_Detection.h"
#define RED  	0xf800
#define GREEN	0x07e0
#define BLUE 	0x001f
#define WHITE	0xffff
#define BLACK	0x0000
#define YELLOW  0xFFE0
#define GRAY0   0xEF7D  //灰色0 3165 00110 001011 00101
#define GRAY1   0x8410  //灰色1      00000 000000 00000
#define GRAY2   0x4208  //灰色2  1111111111011111




#define LCD_CTRLA   	  	GPIOA		//定义TFT数据端口
#define LCD_CTRLB   	  	GPIOB		//定义TFT数据端口



#define LCD_SCL        	GPIO_Pin_6	//PB6--->>TFT --SCL/SCK
#define LCD_SDA        	GPIO_Pin_7	//PB7 MOSI--->>TFT --SDA/DIN
#define LCD_CS        	GPIO_Pin_5  //MCU_PB5--->>TFT --CS/CE

//#define LCD_LED        	GPIO_Pin_10  //DC
#define LCD_RS         	GPIO_Pin_4	//PB11--->>TFT --RS/DC
#define LCD_RST     	GPIO_Pin_15	//PB10--->>TFT --RST

//#define LCD_CS_SET(x) LCD_CTRL->ODR=(LCD_CTRL->ODR&~LCD_CS)|(x ? LCD_CS:0)

//液晶控制口置1操作语句宏定义
#define	LCD_SCL_SET  	LCD_CTRLB->BSRR=LCD_SCL    
#define	LCD_SDA_SET  	LCD_CTRLB->BSRR=LCD_SDA   
#define	LCD_CS_SET  	LCD_CTRLB->BSRR=LCD_CS  

    
#define	LCD_LED_SET  	LCD_CTRLA->BSRR=LCD_LED   
#define	LCD_RS_SET  	LCD_CTRLB->BSRR=LCD_RS 
#define	LCD_RST_SET  	LCD_CTRLA->BSRR=LCD_RST 
//液晶控制口置0操作语句宏定义
#define	LCD_SCL_CLR  	LCD_CTRLB->BRR=LCD_SCL  
#define	LCD_SDA_CLR  	LCD_CTRLB->BRR=LCD_SDA 
#define	LCD_CS_CLR  	LCD_CTRLB->BRR=LCD_CS 
    
#define	LCD_LED_CLR  	LCD_CTRLA->BRR=LCD_LED 
#define	LCD_RST_CLR  	LCD_CTRLA->BRR=LCD_RST
#define	LCD_RS_CLR  	LCD_CTRLB->BRR=LCD_RS 

#define LCD_DATAOUT(x) LCD_DATA->ODR=x; //数据输出
#define LCD_DATAIN     LCD_DATA->IDR;   //数据输入

#define LCD_WR_DATA(data){\
LCD_RS_SET;\
LCD_CS_CLR;\
LCD_DATAOUT(data);\
LCD_WR_CLR;\
LCD_WR_SET;\
LCD_CS_SET;\
} 

extern char buffer[20];

void LCD_GPIO_Init(void);
void Lcd_WriteIndex(u8 Index);
void Lcd_WriteData(u8 Data);
void Lcd_WriteReg(u8 Index,u8 Data);
u16 Lcd_ReadReg(u8 LCD_Reg);
void Lcd_Reset(void);
void Lcd_Init(void);
void Lcd_Clear(u16 Color);
void Lcd_SetXY(u16 x,u16 y);
void Gui_DrawPoint(u16 x,u16 y,u16 Data);
unsigned int Lcd_ReadPoint(u16 x,u16 y);
void Lcd_SetRegion(u16 x_start,u16 y_start,u16 x_end,u16 y_end);
void LCD_WriteData_16Bit(u16 Data);



void Redraw_Mainmenu(void);
void ASCLL_Test(void);
void Font_Test(void);
void Color_Test(void);
void showimage(const unsigned char *p); 
void QDTFT_Test_Demo(void);
void Lcd_Clear_1(u16 Color) ;  


void Display_All_Data(EnvironmentalData_t* env_data);
#endif