//pms.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "reliya.h"

///////////////////////////////////////////////////////////
// RELIYA_Init for HGPC-100
void RELIYA_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_DELAY,20,5);
	Convert_USART_SetRcvMode(USART_RCV_DELAY,20,5);
//	USART3_Configuration(USART_BAUD(115200), PARITY_NONE, STB_1);
    set_speed(fd, 115200);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint8_t RELIYA_HGPC_100_Analysis(uint16_t len)
{
	char *ptr,*eptr;
	uint8_t i;
	float f_value;
	uint32_t u32_value;
	float data[8];
	
	//Check Frame 
	if(serial_buff[len - 1]!= 0x0A 
		|| serial_buff[len - 2]!= 0x0D)
	{
		goto err_exit;
	}
	
	ptr= (char*)&serial_buff[0];
	
	//Find Date Position
	for(i= 0;i<3;i++)
	{
		eptr= strchr(ptr,',');
		if(eptr== NULL)
		{
			goto err_exit;
		}
		ptr= eptr + 1;
	}
	
	//Particle
	for(i= 0;i<5;i++)
	{
		eptr= strchr(ptr,',');
		if(eptr== NULL)
		{
			eptr= strchr(ptr,0x0D);
			if(eptr== NULL)
			{
				goto err_exit;
			}
		}
					
		memset(data_temp,0,20);
		strncpy((char*)data_temp,ptr,eptr - ptr);
		ptr= eptr + 1;
		
		data[i]= atof(data_temp);
	}
	
	for(i= 0;i<4;i++)		
	{
		f_value= data[i];
		
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
	}

	return 0;
	
err_exit:
	return 1;
}
