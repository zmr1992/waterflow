#ifndef __USART1_H
#define	__USART1_H
#include "common.h" 
void uart4_init(u32 bound);
void ESP8266_init(void);
void UART4Test(void);
void UART4SendByte(unsigned char SendData);
unsigned char UART4GetByte(unsigned char* GetData);
void UART4_IRQHandler(void);
void TIM2_Init(u16 auto_data,u16 fractional);
void TimerMs(void);
uint32_t GetTimerCount(void);
void TIM2_IRQHandler(void);

#endif
