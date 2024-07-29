#ifndef __USART_TYPE_H__
#define __USART_TYPE_H__

#include <stdint.h>

typedef enum
{
	USART_RCV_DELAY= 0,
	USART_RCV_CHAR= 1,
	USART_RCV_LEN= 2
}USART_RCM_T;	//Receive Mode

typedef struct
{
	char xon;
	char xoff;
}USART_SFC_DEF;	//Software Flow Control

typedef struct
{
	USART_RCM_T mode;
	union
	{
		USART_SFC_DEF xchar;
		uint16_t len;
		uint16_t delay;
	}param;
	uint8_t rcvd;
	uint16_t data_len;
}USART_RCV_DEF;

void USART_SetRcvMode(USART_RCV_DEF* rcv_set, USART_RCM_T mode, uint16_t par1, uint16_t par2);
void Convert_USART_SetRcvMode(USART_RCM_T mode, uint16_t par1, uint16_t par2);
#endif

