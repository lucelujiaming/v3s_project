//nanochrome.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "jag.h"

#define NANO_CMD_MAX		2

static uint8_t cmd_cnt;

char NANO_Cmd[NANO_CMD_MAX][5]= 
{
	{'B','B'},
	{'C','C'},
};

uint8_t nanochrome_check_sum(uint16_t len)
{
	uint8_t check_sum= 0;
	uint16_t i;
	
	for(i= 0; i< (len-3);i++)
	{
		check_sum+= serial_buff[1+i];
	}
	
	if(check_sum== serial_buff[len - 2])
	{
		return 1;
	}
	
	return 0;
}

void SERVOMEX_NANO_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_CHAR,'>', '<');
	Convert_USART_SetRcvMode(USART_RCV_CHAR,'>', '<');
//	USART3_Configuration(USART_BAUD(38400), PARITY_NONE, STB_1);
    set_speed(fd, 38400);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint16_t SERVOMEX_NANO_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= NANO_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= strlen(NANO_Cmd[cmd_cnt]);
	memmove(serial_buff,(const char*)NANO_Cmd[cmd_cnt],len);
	
	return len;
}

//Concentration Unit:PPB
uint8_t SERVOMEX_NANO_Analysis(uint16_t len)
{
	uint8_t i,groups;
	uint16_t data;
	uint8_t err;
	float f_value;
	uint32_t u32_value;
	uint8_t *buff;
	
	err= 1;
	buff= serial_buff;
	if(nanochrome_check_sum(len))
	{
		err= 0;
		buff++;
		
		switch(cmd_cnt)
		{
		case 0:
			if(*buff != 'B')
			{
				break;
			}
			buff++;
			
			if(*buff == '1')
			{
				buff++;
				data= 0;
				
				for(i=0; i< 9;i++)
				{
					if(*buff == '1')
					{
						data|= (uint16_t)0x01 << i;
					}
					buff++;
				}
				
				protocol_buff[1]= data;
			}
			break;
		case 1:
			if(*buff != 'C')
			{
				break;
			}
			buff++;
			
			if(*buff == '1')
			{
				buff++;
				if(((len - 5)%15)== 0)
				{
					groups= (len - 5)/15;
					
					// Get AI Value
					for(i= 0;i<groups;i++)
					{
						memset(data_temp,0,20);
						strncpy(data_temp,(const char*)buff,10);
						
						f_value= atof(data_temp);
						u32_value= real_to_u32(f_value);
						
						if(little_endian)
						{
							protocol_buff[2 + 4 * i]= _low_word_int32(u32_value);
							protocol_buff[3 + 4 * i]= _high_word_int32(u32_value);
						}
						else
						{
							protocol_buff[2 + 4 * i]= _high_word_int32(u32_value);
							protocol_buff[3 + 4 * i]= _low_word_int32(u32_value);
						}
						buff+= 10;
						
						//
						if(*buff == ';')
						{
							buff++;
							
							protocol_buff[4 + 4*i]= *buff - '0';
							buff++;
						}
						else
						{
							break;
						}
						
						if(*buff == ';')
						{
							buff++;
							
							protocol_buff[5 + 4*i]= *buff- '0';
							buff++;
						}
						else
						{
							break;
						}
						
						if(*buff != ';')
						{
							break;
						}
						buff++;
					}
				}
			}
			break;
		default:
			break;
		}
	}

	return err;
}
