//orthodyne.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "orthodyne.h"

///////////////////////////////////////////////////////////
void ORTHODYNE_Init(int fd, uint16_t addr)
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

uint8_t ORTHODYNE_Analysis(uint16_t len)
{
	uint8_t err;
	uint8_t j, size;
	uint16_t data;
	float f_value;
	uint32_t u32_value;
	char* str_pos,*start_pos,*stop_pos= NULL,*tab_pos,*med_pos;
	uint8_t* str_ptr;
	char crlf[3]= {0x0D,0x0A,0x00};
	char tab[2]= {0x09,0x00};
	
	err= 1;
	
	serial_buff[len]= 0; //Add NULL at tail
	start_pos= (char*)serial_buff;
	stop_pos= strstr((char*)serial_buff,"END");
	
	if(stop_pos!= NULL)
	{
		j= 0;
		
		start_pos= strstr(start_pos,crlf);	//DateTime 1
		start_pos= start_pos+2;
		start_pos= strstr(start_pos,crlf);	//DateTime 2
		start_pos= start_pos+2;
		start_pos= strstr(start_pos,crlf);	//Analyse
		start_pos= start_pos+2;
		
		while(1)
		{
			str_pos= strstr(start_pos,crlf);	//concentation
			
			if(str_pos!= NULL && str_pos< stop_pos)
			{
				//Name
				med_pos= start_pos;
				tab_pos= strstr(med_pos,tab);		
				size= tab_pos - med_pos;
				str_ptr= (uint8_t*)(&protocol_buff[1 + 8 * j]);
				memmove(str_ptr, med_pos, size);
				
				//Concentration
				med_pos= tab_pos+1;
				tab_pos= strstr(med_pos,tab);		
				size= tab_pos-med_pos;
				
				memset(data_temp,0,20);
				strncpy(data_temp,(const char*)(med_pos),size);
				
				f_value= atof(data_temp);
				
				
				//Unit
				med_pos= tab_pos+1;
				tab_pos= strstr(med_pos,tab);
				
				if(med_pos[0]== 'p' && med_pos[1]== 'p' && med_pos[2]== 'm')
				{
					f_value*= 1000;
				}
				
				u32_value= real_to_u32(f_value);
				
				if(little_endian)
				{
					protocol_buff[5 + 8 * j]= _low_word_int32(u32_value);
					protocol_buff[6 + 8 * j]= _high_word_int32(u32_value);
				}
				else
				{
					protocol_buff[5 + 8 * j]= _high_word_int32(u32_value);
					protocol_buff[6 + 8 * j]= _low_word_int32(u32_value);
				}
				//Alarm
				med_pos= tab_pos+1;
				tab_pos= strstr(med_pos,tab);
				size= tab_pos-med_pos;
				
				memset(data_temp,0,20);
				strncpy(data_temp,(const char*)(med_pos),size);
				data= atoi(data_temp);
				protocol_buff[8+8*j]= data;
			}
			else
			{
				break;
			}
			start_pos= str_pos+2;
			j++;
		}
		err= 0;
	}
	
	return err;
}

