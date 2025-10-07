#ifndef __BEEP_H__
#define __BEEP_H__

#include "sys.h"

void Buzzer_Init(void);

void Buzzer_On(void);

void Buzzer_Off(void);

void Buzzer_Alarm(uint8_t times);

#endif

