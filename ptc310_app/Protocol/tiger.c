//tiger.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "tiger.h"

#define TIGER_CMD_MAX		3

static uint8_t cmd_cnt;

char TIGER_Cmd[TIGER_CMD_MAX][10]= 
{
	{'C','O','N','C',0x0D,0X0A},
	{'O','P','M','O','D','E',0x0D,0X0A},
	{'G','A','S','T','Y','P','E',0x0D,0X0A}
};

void TIGER_Init(int fd, uint16_t addr)
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

uint16_t TIGER_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= TIGER_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= strlen(TIGER_Cmd[cmd_cnt]);
	memmove(serial_buff,(const char*)TIGER_Cmd[cmd_cnt],len);
	
	return len;
}

uint8_t TIGER_Analysis(uint16_t len)
{
	uint8_t j, str_size;
	uint8_t err;
	float f_value;
	uint32_t u32_value;
	
	err= 1;
	
	switch(cmd_cnt)
	{
	case 0:
		str_size= 0;
		for(j=0;j<len;j++)
		{
			if((*(serial_buff + j)== '.')
				||(*(serial_buff + j)== '-')
				|| ((*(serial_buff + j)>= '0') && (*(serial_buff + j)<= '9')))
			{
				str_size++;
			}
			else
			{
				break;
			}
		}
		
		if(str_size)
		{
			memset(data_temp,0,20);
			strncpy((char*)data_temp,(const char*)serial_buff,str_size);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			if(little_endian)
			{
				protocol_buff[6]= _low_word_int32(u32_value);
				protocol_buff[7]= _high_word_int32(u32_value);
			}
			else
			{
				protocol_buff[6]= _high_word_int32(u32_value);
				protocol_buff[7]= _low_word_int32(u32_value);
			}
			
			err= 0;
		}
		break;
	case 1:
		if(*(serial_buff)>= '0' && *(serial_buff)<= '3')
		{
			protocol_buff[5]= *(serial_buff)- '0';
			err= 0;
		}
		break;
	case 2:
		str_size= 0;
		for(j=0;j<len;j++)
		{
			if(*(serial_buff+j)< 0x20)
			{
				break;
			}
			str_size++;
		}
		
		if(str_size)
		{
			strncpy((char*)&protocol_buff[1],(const char*)(serial_buff),str_size);
			err= 0;
		}
		break;
	default:
		break;
	}

	return err;
}
