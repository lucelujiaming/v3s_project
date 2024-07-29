//usart.c
#include <stdint.h>
#include <stddef.h>
#include "usart_type.h"

static volatile USART_RCV_DEF RecvSet;

void USART_SetRcvMode(USART_RCV_DEF * rcv_set, USART_RCM_T mode, uint16_t par1, uint16_t par2)
{
	if(rcv_set== NULL)
	{
		return;
	}
	
	rcv_set->mode= mode;
	
	switch(mode)
	{
		case USART_RCV_DELAY:
			rcv_set->param.delay= par1;
			break;
		case USART_RCV_CHAR:
			rcv_set->param.xchar.xon= par1;
			rcv_set->param.xchar.xoff= par2;
			break;
		case USART_RCV_LEN:
			rcv_set->param.len= par1;
			break;
		default:
			break;
	}
	
	rcv_set->data_len= 0;
	rcv_set->rcvd= 0;
}

void Convert_USART_SetRcvMode(USART_RCM_T mode, uint16_t par1, uint16_t par2)
{
	USART_SetRcvMode((USART_RCV_DEF*)&RecvSet, mode, par1, par2);
}

