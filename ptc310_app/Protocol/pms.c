//pms.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "pms.h"

uint8_t pms_check_sum(uint8_t* buf,uint8_t pos)
{
	uint16_t check_sum,sum;
	uint8_t i;
		
	check_sum= 0;
	for(i= 0;i<4;i++)
	{	
		check_sum*= 16;
		if(buf[pos+i]>= 'a')
		{
			check_sum+= buf[pos+i]-'a'+10;
		}
		else
		{
			check_sum+= buf[pos+i]-'0';
		}
	}
	
	sum= 0;
	for(i=0;i<pos;i++)
	{
		sum+= buf[i];
	}
	
	if(sum== check_sum)
	{
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////
// PMS_Init_1 for PDS-PA & HPGH-101
void PMS_Init_1(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_CHAR, 0x02, 0x03);
	Convert_USART_SetRcvMode(USART_RCV_CHAR, 0x02, 0x03);
//	USART3_Configuration(USART_BAUD(1200), PARITY_NONE, STB_1);
    set_speed(fd, 1200);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint8_t PMS_PDS_PA_Analysis(uint16_t len)
{
	uint8_t err;
	uint8_t i, j, pos;
	float f_value;
	uint32_t u32_value;
	
	err= 1;
	
	if(pms_check_sum(serial_buff, len-7))
	{
		data_temp[10]= 0;
		pos= 0;
		j= 0;
		//Find three 0x0A
		while(pos < len)
		{
			if(*(serial_buff + pos++)== 0x0A)
			{
				if(++j >= 3)
				{
					break;
				}
			}
		}
		
		j= serial_buff[pos - 7]-'0'-1;	//Get Probe Number
		
		for(i= 0;i<8;i++)	// Get Particle
		{
			memset(data_temp,0,20);
			strncpy((char*)data_temp,(const char*)(serial_buff + pos),10);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			if(little_endian)
			{
				protocol_buff[1 + 2*i + 50*j]= _low_word_int32(u32_value);
				protocol_buff[2 + 2*i + 50*j]= _high_word_int32(u32_value);
			}
			else
			{
				protocol_buff[1 + 2*i + 50*j]= _high_word_int32(u32_value);
				protocol_buff[2 + 2*i + 50*j]= _low_word_int32(u32_value);
			}
			pos+= 10;
		}
		
		pos+= 2;
		
		for(i= 0;i< 2;i++)	//
		{
			// Get Laser Ref
			memset(data_temp,0,20);
			strncpy(data_temp,(const char*)(serial_buff + pos),10);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			if(little_endian)
			{
				protocol_buff[17 + i*50]= _low_word_int32(u32_value);
				protocol_buff[18 + i*50]= _high_word_int32(u32_value);
			}
			else
			{
				protocol_buff[17 + i*50]= _high_word_int32(u32_value);
				protocol_buff[18 + i*50]= _low_word_int32(u32_value);
			}
			
			pos+= 10;
		}
				
		// Get AI Value
		for(i= 0;i<6;i++)
		{
			memset(data_temp,0,20);
			strncpy(data_temp,(const char*)(serial_buff + pos),10);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			if(little_endian)
			{
				protocol_buff[19 + 2*i]= _low_word_int32(u32_value);
				protocol_buff[20 + 2*i]= _high_word_int32(u32_value);
			}
			else
			{
				protocol_buff[19 + 2*i]= _high_word_int32(u32_value);
				protocol_buff[20 + 2*i]= _low_word_int32(u32_value);
			}
			pos+= 10;
		}
		
		err= 0;
	}
	
	return err;
}

//////////////////////////////////////////////////////////////////
uint8_t PMS_HPGP_101_Analysis(uint16_t len)
{
	uint8_t err;
	uint8_t i, j, pos;
	float f_value;
	uint32_t u32_value;
	
	err= 1;
	
	if(pms_check_sum(serial_buff, len-7))
	{
		data_temp[10]= 0;
		pos= 0;
		j= 0;
		//Find five 0x0A
		while(pos < len)
		{
			if(*(serial_buff + pos++)== 0x0A)
			{
				if(++j >= 5)
				{
					break;
				}
			}
		}
		
		//Value
		for(i= 0;i<8;i++)
		{
			memset(data_temp,0,20);
			strncpy(data_temp,(const char*)(serial_buff + pos),10);
			
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
			
			pos+= 10;
		}
		
		pos+= 2;
		
		for(i= 0;i<7;i++)
		{
			memset(data_temp,0,20);
			strncpy(data_temp,(const char*)(serial_buff + pos),10);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			if(little_endian)
			{
				protocol_buff[17 + 2*i]= _low_word_int32(u32_value);
				protocol_buff[18 + 2*i]= _high_word_int32(u32_value);
			}
			else
			{
				protocol_buff[17 + 2*i]= _high_word_int32(u32_value);
				protocol_buff[18 + 2*i]= _low_word_int32(u32_value);
			}
			pos+= 10;
		}
		
		err= 0;
	}
	
	return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// PMS_Init_2 for Lasair III
void PMS_Init_2(int fd, uint16_t addr)
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

uint8_t PMS_LASAIR_III_Analysis(uint16_t len)
{
	char *ptr,*eptr;
	uint8_t i;
	float f_value;
	uint32_t u32_value;
	
	ptr= (char*)&serial_buff[0];
	
	for(i= 0;i<9;i++)
	{
		eptr= strchr(ptr,',');
		if(eptr== NULL)
		{
			goto err_exit;
		}
		ptr= eptr + 1;
	}
	//Laser OK
	if(*ptr== '0')
	{
		protocol_buff[15]= 0;
	}
	else
	{
		protocol_buff[15]= 1;
	}
	//

	for(i= 0;i<7;i++)
	{
		eptr= strchr(ptr,',');
		if(eptr== NULL)
		{
			goto err_exit;
		}
		ptr= eptr + 1;
	}
	//size 1
	for(i= 0;i<6;i++)
	{
		eptr= strchr(ptr,',');
		if(eptr== NULL)
		{
			goto err_exit;
		}
					
		memset(data_temp,0,20);
		strncpy((char*)data_temp,ptr,eptr - ptr);
		ptr= eptr + 1;
		
		if(strstr(data_temp,"0.3")!= NULL)
		{
			eptr= strchr(ptr,',');
			if(eptr== NULL)
			{
				goto err_exit;
			}
						
			memset(data_temp,0,20);
			strncpy(data_temp,ptr,eptr - ptr);
			
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
		}
		else
		{
			if(strstr(data_temp,"0.5")!= NULL)
			{
				eptr= strchr(ptr,',');
				if(eptr== NULL)
				{
					goto err_exit;
				}
								
				memset(data_temp,0,20);
				strncpy(data_temp,ptr,eptr - ptr);
				
				f_value= atof(data_temp);
				u32_value= real_to_u32(f_value);
				
				if(little_endian)
				{
					protocol_buff[3]= _low_word_int32(u32_value);
					protocol_buff[4]= _high_word_int32(u32_value);
				}
				else
				{
					protocol_buff[3]= _high_word_int32(u32_value);
					protocol_buff[4]= _low_word_int32(u32_value);
				}
			}
			else
			{
				if(strstr(data_temp,"1.0")!= NULL)
				{
					eptr= strchr(ptr,',');
					if(eptr== NULL)
					{
						goto err_exit;
					}
										
					memset(data_temp,0,20);
					strncpy(data_temp,ptr,eptr - ptr);
					
					f_value= atof(data_temp);
					u32_value= real_to_u32(f_value);
					
					if(little_endian)
					{
						protocol_buff[5]= _low_word_int32(u32_value);
						protocol_buff[6]= _high_word_int32(u32_value);
					}
					else
					{
						protocol_buff[5]= _high_word_int32(u32_value);
						protocol_buff[6]= _low_word_int32(u32_value);
					}
				}
				else
				{
					if(strstr(data_temp,"2.0")!= NULL)
					{
						eptr= strchr(ptr,',');
						if(eptr== NULL)
						{
							goto err_exit;
						}
												
						memset(data_temp,0,20);
						strncpy(data_temp,ptr,eptr - ptr);
						
						f_value= atof(data_temp);
						u32_value= real_to_u32(f_value);
						
						if(little_endian)
						{
							protocol_buff[7]= _low_word_int32(u32_value);
							protocol_buff[8]= _high_word_int32(u32_value);
						}
						else
						{
							protocol_buff[7]= _high_word_int32(u32_value);
							protocol_buff[8]= _low_word_int32(u32_value);
						}
					}
					else
					{
						if(strstr(data_temp,"5.0")!= NULL)
						{
							eptr= strchr(ptr,',');
							if(eptr== NULL)
							{
								goto err_exit;
							}
							
							memset(data_temp,0,20);
							strncpy(data_temp,ptr,eptr - ptr);
							
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
						else
						{
							if(strstr(data_temp,"10.0")!= NULL)
							{
								eptr= strchr(ptr,',');
								if(eptr== NULL)
								{
									goto err_exit;
								}
								
								memset(data_temp,0,20);
								strncpy(data_temp,ptr,eptr - ptr);
								
								f_value= atof(data_temp);
								u32_value= real_to_u32(f_value);
								
								if(little_endian)
								{
									protocol_buff[11]= _low_word_int32(u32_value);
									protocol_buff[12]= _high_word_int32(u32_value);
								}
								else
								{
									protocol_buff[11]= _high_word_int32(u32_value);
									protocol_buff[12]= _low_word_int32(u32_value);
								}	
							}
							else
							{
								if(strstr(data_temp,"25.0")!= NULL)
								{
									eptr= strchr(ptr,',');
									if(eptr== NULL)
									{
										goto err_exit;
									}
									
									memset(data_temp,0,20);
									strncpy(data_temp,ptr,eptr - ptr);
									
									f_value= atof(data_temp);
									u32_value= real_to_u32(f_value);
									if(little_endian)
									{
										protocol_buff[13]= _low_word_int32(u32_value);
										protocol_buff[14]= _high_word_int32(u32_value);
									}
									else
									{
										protocol_buff[13]= _high_word_int32(u32_value);
										protocol_buff[14]= _low_word_int32(u32_value);
									}	
								}
							}
						}
					}
				}
			}
		}
		ptr= eptr + 1;
	}

	return 0;
	
err_exit:
	return 1;
}
///////////////////////////////////////////////////////////////////////////////////
uint8_t PMS_PDS_E_Analysis(uint16_t len)		//PMS Mode
{
	char *ptr,*eptr;
	uint8_t i;
	float f_value;
	uint32_t u32_value;
	float data[8];
	
	if(serial_buff[0]!= 'D' 
		|| serial_buff[len - 1]!= 0x0A)
	{
		goto err_exit;
	}
	
	ptr= (char*)&serial_buff[0];
	
	//Find Laser Ref Position
	for(i= 0;i<3;i++)
	{
		eptr= strchr(ptr,',');
		if(eptr== NULL)
		{
			goto err_exit;
		}
		ptr= eptr + 1;
	}

	//Laser Ref
	eptr= strchr(ptr,',');
	if(eptr== NULL)
	{
		goto err_exit;
	}
				
	memset(data_temp,0,20);
	strncpy((char*)data_temp, ptr, eptr - ptr);
	ptr= eptr + 1;
	
	f_value= atof(data_temp);
	u32_value= real_to_u32(f_value);
		
	if(little_endian)
	{
		protocol_buff[17]= _low_word_int32(u32_value);
		protocol_buff[18]= _high_word_int32(u32_value);
	}
	else
	{
		protocol_buff[17]= _high_word_int32(u32_value);
		protocol_buff[18]= _low_word_int32(u32_value);
	}
	
	//Find Particles Position
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
	for(i= 0;i<8;i++)
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
	
	for(i= 0;i<8;i++)		
	{
		if(i== 7)
		{
			f_value= data[7];
		}
		else
		{
			f_value= data[i] - data[i+1];
		}
		
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

