//saes.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "saes.h"

void SAES_Init(int fd, uint16_t addr)
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

//Concentration Unit:PPB
uint8_t SAES_Analysis(uint16_t len)
{
	uint8_t j,size;
	float f_value;
	uint32_t u32_value;
	uint8_t* str_ptr;
	char *start_pos,*stop_pos= NULL,*med_pos;
	char crlf[3]= {0x0D,0x0A,0x00};
	char tab[2]= {0x00,0x00};
	
	j= 0;
	
	serial_buff[len]= 0; //Add NULL at tail
	start_pos= (char*)serial_buff;
	stop_pos= strstr((char*)serial_buff,crlf);
	
	if(stop_pos!= NULL)
	{
		//Year
		tab[0]= '-';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}

		//Month
		tab[0]= '-';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}

		//Date
		tab[0]= ' ';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}
			
		//Hour
		tab[0]= ':';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}
			
		//Minute
		tab[0]= ':';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}
		
		//Second
		tab[0]= ',';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}
		
		//#
		tab[0]= ',';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}
		
		//R/M, 30002
		if((*start_pos)== 'M')
		{
			protocol_buff[1]= 1;
		}
		else
		{
			if((*start_pos)== 'R')
			{
				protocol_buff[1]= 0;
			}
			else
			{
				goto ERROR_EXIT;
			}
		}
		start_pos+= 2;
		
		//E/S, 30003
		if((*start_pos)== 'S')
		{
			protocol_buff[2]= 1;
		}
		else
		{
			if((*start_pos)== 'E')
			{
				protocol_buff[2]= 0;
			}
			else
			{
				goto ERROR_EXIT;
			}
		}
		start_pos+= 2;
		
		//NC
		tab[0]= ',';
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			start_pos= med_pos+1;
		}
		else
		{
			goto ERROR_EXIT;
		}
		
		//Alarm, 30004
		tab[0]= ',';
		protocol_buff[3]= 0;
		med_pos= strstr((char*)start_pos,tab);
		if(med_pos!= NULL && med_pos< stop_pos)
		{
			size= med_pos-start_pos;
			if(size== 8)
			{
				for(j= 0;j<8;j++)
				{
					if((*(start_pos+j))== '1')
					{
						protocol_buff[3]|= 0x01 << (7 - j);
					}
				}
				start_pos= med_pos+1;
			}
			else
			{
				goto ERROR_EXIT;
			}
		}
		else
		{
			goto ERROR_EXIT;
		}
		
		//Name & Concern
		tab[0]= ',';
		for(j= 0;j< 6;j++)
		{
			//Name, 30007
			str_ptr= (uint8_t*)(&protocol_buff[6 + 5*j]);
			memset(str_ptr,0,6);

			med_pos= strstr(start_pos,tab);
			if(med_pos!= NULL && med_pos< stop_pos)
			{
				size= med_pos-start_pos;
				strncpy((char*)str_ptr,(const char*)(start_pos),size);
				start_pos= med_pos + 1;
			}
			else
			{
				goto ERROR_EXIT;
			}

			//Concern	,30010
			if(j== 5)
			{
				med_pos= strstr(start_pos,crlf);
			}
			else
			{
				med_pos= strstr(start_pos,tab);
			}
			
			if(med_pos!= NULL && med_pos<= stop_pos)
			{
				size= med_pos-start_pos;
				memset(data_temp,0,20);
				strncpy(data_temp,(const char*)(start_pos),size);
				
				f_value= atof(data_temp);
				u32_value= real_to_u32(f_value);
				
				if(little_endian)
				{
					protocol_buff[9 + 5*j]= _low_word_int32(u32_value);
					protocol_buff[10 + 5*j]= _high_word_int32(u32_value);
				}
				else
				{
					protocol_buff[9 + 5*j]= _high_word_int32(u32_value);
					protocol_buff[10 + 5*j]= _low_word_int32(u32_value);
				}
				start_pos= med_pos + 1;
			}
			else
			{
				goto ERROR_EXIT;
			}	
		}
	}
	else
	{
		goto ERROR_EXIT;
	}

	return 0;
	
ERROR_EXIT:	
	return 1;
}
