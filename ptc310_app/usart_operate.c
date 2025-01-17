#include <stdio.h>
#include <unistd.h>

#include <termios.h>

#include <stdint.h>
#include <stddef.h>

#include "protocol.h"
#include "usart_operate.h"


// static volatile uint8_t DataBuff[256];
static uint8_t InstrumentDataBuff[DATA_LENGTH];
// static volatile uint16_t send_len= 0;
// static volatile uint16_t send_count= 0;
// static volatile uint16_t recv_count= 0;
// static volatile uint8_t send_complete= 0;
// static volatile uint8_t interface;

static volatile uint8_t send_ret;

// 初始化为非循环定时器，未使能，超时时间为3毫秒。
static volatile MTIMER tm_ByteOvr     = {"ByteOvr", 0, false, false, {0, 0}, 3};
// 初始化为非循环定时器，未使能，超时时间为5毫秒。
static volatile MTIMER tm_FrmRcvDelay = {"FrmRcvDelay", 0, false, false, {0, 0}, 5};
static volatile USART_RCV_DEF RecvSet;

void Instrument_USART_Init()
{	
	Timer_Init((MTIMER*)&tm_ByteOvr);
	Timer_Init((MTIMER*)&tm_FrmRcvDelay);
	
	Timer_SetParam((MTIMER*)&tm_ByteOvr, true, 3);
	Timer_Restart((MTIMER*)&tm_ByteOvr);
	Timer_SetParam((MTIMER*)&tm_FrmRcvDelay, true, 5);
	Timer_Restart((MTIMER*)&tm_FrmRcvDelay);
}

void Instrument_USART_Send(int fd, uint16_t len)
{
    send_ret = write(fd, InstrumentDataBuff, len);
	// 等待数据发送完成
	tcdrain(fd);
}
uint16_t Instrument_USART_FrameReceived(int fd)
{
	// printf("Timer_Expires tm_FrmRcvDelay\n");
	if(Timer_Expires((MTIMER*)&tm_FrmRcvDelay))
	{
	//	RecvSet.data_len= recv_count;
		RecvSet.rcvd= 1;
	//	recv_count= 0;
	}
	//	printf("read UUUUUUUU\n");
	if(RecvSet.rcvd)
	{
		RecvSet.rcvd= 0;
		// printf("Instrument_read read\n");
    	return read(fd, InstrumentDataBuff, DATA_LENGTH);
	}
	return 0;
}
uint8_t *Instrument_USART_GetBuf()
{
    return InstrumentDataBuff;
}

uint8_t Instrument_USART_SendComplete(int fd)
{
    return 1;
}

void USART_SetRcvMode(USART_RCV_DEF * rcv_set, USART_RCM_T mode, uint16_t par1, uint16_t par2)
{
	if(rcv_set== NULL)
	{
		printf("USART_SetRcvMode rcv_set== NULL\n");
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

void Instrument_USART_SetRcvMode(USART_RCM_T mode, uint16_t par1, uint16_t par2)
{
	USART_SetRcvMode((USART_RCV_DEF*)&RecvSet, mode, par1, par2);
	if(mode == USART_RCV_DELAY)
	{
		Timer_SetParam((MTIMER*)&tm_FrmRcvDelay, false, par1);
		Timer_SetParam((MTIMER*)&tm_ByteOvr, false, par2);
	}
}


