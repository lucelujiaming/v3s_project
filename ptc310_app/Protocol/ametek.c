//ametek.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "jag.h"

#define AMETEK_2850_CMD_MAX		4

static uint8_t cmd_cnt;

char AMETEK_2850_Cmd[AMETEK_2850_CMD_MAX][7]= 
{
	{0xC9,'R','0','1',0x0D,0x0A},
	{0xC9,'R','0','6',0x0D,0x0A},
	{0xC9,'R','1','0',0x0D,0x0A},
	{0xC9,'R','1','1',0x0D,0x0A}
};

void AMETEK_Init(int fd, uint16_t addr)
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

uint16_t AMETEK_2850_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= AMETEK_2850_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= strlen(AMETEK_2850_Cmd[cmd_cnt]);
	memmove(serial_buff,(const char*)AMETEK_2850_Cmd[cmd_cnt], len);
	
	return len;
}

uint8_t AMETEK_2850_Analysis(uint16_t len)
{
	uint16_t data_1, data_2;
	uint8_t err;
	float f_value;
	uint32_t u32_value;
	
	err= 1;
	
	if(serial_buff[0]== 0x20 && serial_buff[len - 2]== 0x0D && serial_buff[len - 1]== 0x11)
	{
		err= 0;
		
		memset(data_temp,0,20);
		strncpy(data_temp,(char*)&serial_buff[1],len-3);
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
		if(little_endian)
		{
			data_1= _low_word_int32(u32_value);
			data_2= _high_word_int32(u32_value);
		}
		else
		{
			data_1= _high_word_int32(u32_value);
			data_2= _low_word_int32(u32_value);
		}
		
		switch(cmd_cnt)
		{
		case 0:
			protocol_buff[1]= data_1;
			protocol_buff[2]= data_2;
			break;
		case 1:
			protocol_buff[3]= data_1;
			protocol_buff[4]= data_2;
			break;
		case 2:
			protocol_buff[5]= data_1;
			protocol_buff[6]= data_2;
			break;
		case 3:
			protocol_buff[7]= data_1;
			protocol_buff[8]= data_2;
			break;
		default:
			break;
		}
	}

	return err;
}
/////////////////////////////////////////////
uint16_t AMETEK_5000_Request(void)
{
	uint8_t len;
	
	len= 10;
	strncpy((char*)serial_buff, ">F0F5B33\r\n", len);
		
	return len;
}

uint8_t AMETEK_5000_Analysis(uint16_t len)
{
	uint8_t err;
	float f_value;
	uint32_t u32_value;
	
	err= 1;
	
	if(serial_buff[0]== 'A' && serial_buff[len - 1]== 0x0D)
	{
		err= 0;
		memset(data_temp,0,20);
		strncpy(data_temp,(const char*)(serial_buff + 1),len - 4);
		
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
		if(little_endian)
		{
			protocol_buff[9]= _low_word_int32(u32_value);
			protocol_buff[10]= _high_word_int32(u32_value);
		}
		else
		{
			protocol_buff[9]= _high_word_int32(u32_value);
			protocol_buff[10]= _low_word_int32(u32_value);
		}
	}

	return err;
}
