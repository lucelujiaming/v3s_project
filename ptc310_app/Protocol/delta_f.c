//delta_f.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "delta_f.h"

#define DELTAF_CMD_MAX		2

static uint8_t cmd_cnt;

char DELTAF_Cmd[DELTAF_CMD_MAX][8]= 
{
	{0x01,0x01,0x00,0x00,0x00,0x01,0x0D},
	{0x01,0x01,0x01,0x00,0x00,0x02,0x0D},
//	{0x01,0x01,0x66,0x00,0x00,0x67,0x0D}
};

uint8_t deltaf_check_sum(uint8_t len)
{
	uint8_t i;
	uint16_t sum= 0;
	
	for(i= 0;i< len - 4;i++)
	{
		sum+= *(serial_buff + i + 1);
	}
	
	if(sum== _get_int16_int8_big_endian(serial_buff + len - 3))
	{
		return 1;
	}
	
	return 0;
}

void DELTAF_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_DELAY,50,10);
    Convert_USART_SetRcvMode(USART_RCV_DELAY,50,10);
//	USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
    set_speed(fd, 9600);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint16_t DELTAF_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= DELTAF_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= 7;
	memmove(serial_buff,(const char*)DELTAF_Cmd[cmd_cnt],len);
	
	return len;
}

uint8_t DELTAF_Analysis(uint16_t len)
{
	uint8_t err;
	
	err= 1;
	
	if(deltaf_check_sum(len))
	{
		switch(*(serial_buff + 2))
		{
		case 0x00:
			protocol_buff[1]= *(serial_buff+4);
			protocol_buff[2]= *(serial_buff+5);
			protocol_buff[3]= *(serial_buff+6);
			break;
		case 0x01:
			if(little_endian)
			{
				protocol_buff[5]= _get_int16_int8_big_endian(serial_buff+4);
				protocol_buff[4]= _get_int16_int8_big_endian(serial_buff+6); 
			}
			else
			{
				protocol_buff[4]= _get_int16_int8_big_endian(serial_buff+4);
				protocol_buff[5]= _get_int16_int8_big_endian(serial_buff+6); 
			}
			break;
		case 0x66:
			if(little_endian)
			{
				protocol_buff[7]= _get_int16_int8_big_endian(serial_buff+7);
				protocol_buff[6]= _get_int16_int8_big_endian(serial_buff+9); 
			}
			else
			{
				protocol_buff[6]= _get_int16_int8_big_endian(serial_buff+7);
				protocol_buff[7]= _get_int16_int8_big_endian(serial_buff+9); 
			}
			break;
		default:
			break;
		}
		err= 0;
	}

	return err;
}
