//teledyne.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "teledyne.h"

///////////////////////////////////////////////////////////
void TELEDYNE_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_CHAR, 'S', 0x00);
	Convert_USART_SetRcvMode(USART_RCV_CHAR, 'S', 0x00);
//	USART3_Configuration(USART_BAUD(2400), PARITY_NONE, STB_1);
    set_speed(fd, 2400);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint8_t TELEDYNE_Analysis(uint16_t len)
{
	uint8_t err;
	uint8_t size;
	float f_value;
	uint32_t u32_value;
	char *start_pos,*stop_pos= NULL;
	
	err= 1;
	
	if(serial_buff[0]!= 'S' || serial_buff[1]!= 'T' || serial_buff[len - 1]!= 0)
	{
		return 1;
	}
	
	serial_buff[len]= 0;	//Add NULL at tail
	start_pos= strstr((char*)serial_buff,",");
	start_pos++;
	stop_pos= strstr(start_pos,",");
	
	if(start_pos!= NULL && stop_pos!= NULL && stop_pos > start_pos)
	{
		size= stop_pos - start_pos;
		
		memcpy(data_temp,start_pos,size);
		data_temp[size]= 0;
		
		stop_pos= strstr(start_pos,"%");
		if(stop_pos!= NULL)
		{
			protocol_buff[3]= 1;
		}
		else
		{
			stop_pos= strstr(start_pos,"ppm");
			if(stop_pos!= NULL)
			{
				protocol_buff[3]= 2;
			}
			else
			{
				stop_pos= strstr(start_pos,"ppb");
				if(stop_pos!= NULL)
				{
					protocol_buff[3]= 3;
				}
				else
				{
					protocol_buff[3]= 0;
				}
			}
		}
		
		if(protocol_buff[3])
		{
			size= stop_pos - start_pos;
			memcpy(data_temp,start_pos,size);
			data_temp[size]= 0;
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			if(little_endian)
			{
				protocol_buff[1]= _low_word_int32(u32_value);
				protocol_buff[2]= _high_word_int32(u32_value);
			}
			else
			{
				protocol_buff[1]= _high_word_int32(u32_value);
				protocol_buff[2]= _low_word_int32(u32_value);
			}

			err= 0;
		}
	}
	
	return err;
}

