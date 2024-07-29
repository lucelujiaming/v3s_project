//peak.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "peak.h"

static uint16_t req_addr;

void PEAK_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_CHAR,0x02,0x03);
	Convert_USART_SetRcvMode(USART_RCV_CHAR,0x02,0x03);
//	USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
    set_speed(fd, 9600);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
	req_addr= addr;
}

uint16_t PEAK_Request(void)
{
	uint8_t len;
	
	len= 0;
	if(req_addr)
	{
		serial_buff[0]= req_addr/100+'0';
		serial_buff[1]= (req_addr%100)/10+'0';
		serial_buff[2]= req_addr%10+'0';
		
		len= 3;
	}
	
	return len;
}

//Concentration Unit:PPB
uint8_t PEAK_Analysis(uint16_t len)
{
	uint8_t data_pos;
	uint8_t i,j,str_size;
	uint16_t data;
	uint8_t err;
	float f_value;
	uint32_t u32_value;
	uint8_t* str_ptr;
	
	err= 1;
	i= 0;
	j= 0;
	data_pos= 0;
	
	//Find position after 3 ','
	while(data_pos < len)
	{
		if(*(serial_buff + data_pos++)==',')
		{
			if(++j>= 3)
			{
				break;
			}
		}
	}
	
	//Get General Error
	str_size= 0;
	for(j= 0;j< len- data_pos;j++)
	{
		if(*(serial_buff + data_pos + j)== ',')
		{
			break;
		}
		str_size++;
	}

	if(str_size)
	{
		memset(data_temp,0,20);
		strncpy((char*)data_temp,(const char*)(serial_buff+data_pos),str_size);
		data= atoi(data_temp);
		
		protocol_buff[1]= data;
		data_pos+= str_size + 1;
	}
	else
	{
		data_pos++;
	}
	
	//Get Stream Number
	str_size= 0;
	for(j= 0;j< len-data_pos;j++)
	{
		if(*(serial_buff+data_pos+j)== ',')
		{
			break;
		}
		str_size++;
	}

	if(str_size)
	{
		memset(data_temp,0,20);
		strncpy((char*)data_temp,(const char*)(serial_buff+data_pos),str_size);
		data= atoi(data_temp);
		
		protocol_buff[2]= data;
		data_pos+= str_size + 1;
	}
	else
	{
		data_pos++;
	}
	
	//Get Name , Area and Concent
	for(i= 0;i< 4;i++)
	{
		//Get Name
		str_size= 0;
		for(j= 0;j< len- data_pos;j++)
		{
			if(*(serial_buff+data_pos+j)== ',')
			{
				break;
			}
			str_size++;
		}
		
		if(str_size)
		{
			str_ptr= (uint8_t*)(&protocol_buff[3 + i* 8]);
			for(j= 0;j< str_size;j++)
			{
				str_ptr[j]=  *(serial_buff + data_pos + j);
			}
			data_pos+= str_size + 1;
		}
		else
		{
			data_pos++;
		}
		//Get Area
		str_size= 0;
		for(j= 0;j< len- data_pos;j++)
		{
			if(*(serial_buff + data_pos + j)== ',')
			{
				break;
			}
			str_size++;
		}
		
		if(str_size)
		{
			for(j= 0;j< str_size;j++)
			{
				memset(data_temp,0,20);
				strncpy((char*)data_temp,(const char*)(serial_buff+data_pos),str_size);
				
				f_value= atof(data_temp);
				u32_value= real_to_u32(f_value);
				
				if(little_endian)
				{
					protocol_buff[7 + 8 * i]= _low_word_int32(u32_value);
					protocol_buff[8 + 8 * i]= _high_word_int32(u32_value);
				}
				else
				{
					protocol_buff[7 + 8 * i]= _high_word_int32(u32_value);
					protocol_buff[8 + 8 * i]= _low_word_int32(u32_value);
				}
			}
			data_pos+= str_size + 1;
		}
		else
		{
			data_pos++;
		}
		
		//Get Concent
		str_size= 0;
		for(j= 0;j< len-data_pos;j++)
		{
			if(*(serial_buff + data_pos + j)== ',')
			{
				break;
			}
			str_size++;
		}
		
		if(str_size)
		{
			for(j= 0;j< str_size;j++)
			{
				memset(data_temp,0,20);
				strncpy((char*)data_temp,(const char*)(serial_buff + data_pos),str_size);
				
				f_value= atof(data_temp);
				u32_value= real_to_u32(f_value);
				
				if(little_endian)
				{
					protocol_buff[9 + 8 * i]= _low_word_int32(u32_value);
					protocol_buff[10 + 8 * i]= _high_word_int32(u32_value);
				}
				else
				{
					protocol_buff[9 + 8 * i]= _high_word_int32(u32_value);
					protocol_buff[10 + 8 * i]= _low_word_int32(u32_value);
				}
			}
			data_pos+= str_size + 1;
		}
		else
		{
			data_pos++;
		}
	}
	
	err= 0;

	return err;
}
