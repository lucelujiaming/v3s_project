//pms.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "hctm.h"

///////////////////////////////////////////////////////////
// PMS_Init_1 for PDS-PA & HPGH-101
void HCTM_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_CHAR, 0x02, 0x03);
	Convert_USART_SetRcvMode(USART_RCV_CHAR, 0x02, 0x03);
//	USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
    set_speed(fd, 9600);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint8_t HCTM_WCPC0703E_Analysis(uint16_t len)
{
	uint8_t i, j, pos;
	float f_value;
	uint32_t u32_value;
	char *ptr = 0 ,*eptr;
	
	data_temp[10]= 0;

	//Find three 0x0A
	pos= 0;
	j= 0;
	while(pos < len)
	{
		if(*(serial_buff + pos)== 0x0D 
			&& *(serial_buff + pos + 1)== 0x0A)
		{
			if(++j == 3)
			{
				ptr= (char*)&serial_buff[pos + 2];
			}
		}
		pos++;
	}

	if(ptr == 0)
		return 0;
	
	// Error
	if(j!= 15)
		return 1;
	
	
	for(i= 0;i< 3; i++)	// Get Particle
	{
		eptr= strstr(ptr, "\r\n");
		if(eptr== NULL)
		{
			return 1;
		}
		
		memset(data_temp,0,20);
		strncpy((char*)data_temp, ptr, eptr-ptr);
		
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
		if(little_endian)
		{
			protocol_buff[1 + 2*i]= _low_word_int32(u32_value);
			protocol_buff[2 + 2*i]= _high_word_int32(u32_value);
		}
		else
		{
			protocol_buff[1 + 2*i]= _high_word_int32(u32_value);
			protocol_buff[2 + 2*i]= _low_word_int32(u32_value);
		}
		
		ptr= eptr+ 2;
	}

	return 0;
}

