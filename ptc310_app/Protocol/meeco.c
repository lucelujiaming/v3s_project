//meeco.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "meeco.h"

#define MEECO_CMD_MAX		4

static uint8_t cmd_cnt;

char MEECO_Cmd[MEECO_CMD_MAX][5]= 
{
	{'I','4',0x0D},
	{'I','1','0',0x0D},
	{'F','5',0x0D},
	{'F','8',0x0D}
};

void MEECO_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_DELAY,50,10);
    Instrument_USART_SetRcvMode(USART_RCV_DELAY,50,10);
//	USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
    set_speed(fd, 9600);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint16_t MEECO_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= MEECO_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= strlen(MEECO_Cmd[cmd_cnt]);
	memmove(serial_buff,(const char*)MEECO_Cmd[cmd_cnt],len);
	
	return len;
}

uint8_t MEECO_Analysis(uint16_t len)
{
	char* pos;
	uint8_t i,str_size;
	uint16_t data;
	uint8_t err;
	float f_value;
	uint32_t u32_value;
	
	err= 1;
	
	switch(cmd_cnt)
	{
	case 0:
		str_size= 0;
		if(strncmp((const char*)serial_buff, (const char*)MEECO_Cmd[0],2)== 0)
		{
			pos= strstr((const char*)serial_buff,"=");
			
			if(pos== NULL)
			{
				break;
			}
			else
			{
				if(*(pos-2)!= MEECO_Cmd[0][0] || *(pos-1)!= MEECO_Cmd[0][1])
				{
					break;
				}
			}
			
			for(i=0; i< len- 3; i++)
			{
				str_size++;
				if(*(pos+1+i)== 0x0D)
				{
					break;
				}
			}
			if(str_size)
			{
				memset(data_temp,0,20);
				strncpy((char*)data_temp,(const char*)(pos+1),str_size);
				data= atoi(data_temp);
				
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
				protocol_buff[1]= data;
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			}
			err= 0;
		}
		break;
	case 1:
		str_size= 0;
		if(strncmp((const char*)serial_buff,(const char*)MEECO_Cmd[1],3)== 0)
		{
			pos= strstr((const char*)serial_buff,"=");
			
			if(pos== NULL)
			{
				break;
			}
			else
			{
				if(*(pos-3)!= MEECO_Cmd[1][0] || *(pos-2)!= MEECO_Cmd[1][1] || *(pos-1)!= MEECO_Cmd[1][2])
				{
					break;
				}
			}
			
			for(i=0;i<len-4;i++)
			{
				str_size++;
				if(*(pos+1+i)== 0x0D)
				{
					break;
				}
			}
			if(str_size)
			{
				memset(data_temp,0,20);
				strncpy((char*)data_temp,(const char*)(pos+1),str_size);
				data= atoi(data_temp);
				
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
				protocol_buff[2]= data;
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			}
			err= 0;
		}
		break;
	case 2:
		str_size= 0;
		if(strncmp((const char*)serial_buff,(const char*)MEECO_Cmd[2],2)== 0)
		{
			pos= strstr((const char*)serial_buff,"=");
			
			if(pos== NULL)
			{
				break;
			}
			else
			{
				if(*(pos-2)!= MEECO_Cmd[2][0] || *(pos-1)!= MEECO_Cmd[2][1])
				{
					break;
				}
			}
			
			for(i=0;i<len-3;i++)
			{
				str_size++;
				if(*(pos+1+i)== 0x0D)
				{
					break;
				}
			}
			
			if(str_size)
			{
				memset(data_temp,0,20);
				strncpy((char*)data_temp,(const char*)(pos+1),str_size);
				
				f_value= atof(data_temp);
				u32_value= real_to_u32(f_value);
				
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			}
			err= 0;
		}
		break;
	case 3:
		str_size= 0;
		if(strncmp((const char*)serial_buff,(const char*)MEECO_Cmd[3],2)== 0)
		{
			pos= strstr((const char*)serial_buff,"=");
			
			if(pos== NULL)
			{
				break;
			}
			else
			{
				if(*(pos-2)!= MEECO_Cmd[3][0] || *(pos-1)!= MEECO_Cmd[3][1])
				{
					break;
				}
			}
			
			for(i=0;i<len-3;i++)
			{
				str_size++;
				if(*(pos+1+i)== 0x0D)
				{
					break;
				}
			}
			if(str_size)
			{
				memset(data_temp,0,20);
				strncpy((char*)data_temp,(const char*)(pos+1),str_size);
				
				f_value= atof(data_temp);
				u32_value= real_to_u32(f_value);
				
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			}
			err= 0;
		}
		break;
	default:
		break;
	}

	return err;
}

/*
   MEECO：Tracer2 ，Aquavolt
   寄存器     类型      功能描述    
   30001   Int     Communication Fault
   30002   Int     Operation Mode  
   30003   Int     Alarm Message   
   30004   Real    Display Value   
   30006   Real    Flow Setpoint
 */
uint16_t MEECO_DataOutput(char* strOutput)
{
	char cDisplayValueBuffer[4] = {0};
	char cFlowSetpointBuffer[4] = {0};
	float * floatDisplayValue  = (float *)cDisplayValueBuffer;
	float * floatFlowSetpoint = (float *)cFlowSetpointBuffer;
    
    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cDisplayValueBuffer[3] = protocol_buff[4]>>8;
		cDisplayValueBuffer[2] = protocol_buff[4]&0x00FF;
		cDisplayValueBuffer[1] = protocol_buff[3]>>8;
		cDisplayValueBuffer[0] = protocol_buff[3]&0x00FF;
		
		cFlowSetpointBuffer[3] = protocol_buff[6]>>8;
		cFlowSetpointBuffer[2] = protocol_buff[6]&0x00FF;
		cFlowSetpointBuffer[1] = protocol_buff[5]>>8;
		cFlowSetpointBuffer[0] = protocol_buff[5]&0x00FF;
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cDisplayValueBuffer[3] = protocol_buff[3]>>8;
		cDisplayValueBuffer[2] = protocol_buff[3]&0x00FF;
		cDisplayValueBuffer[1] = protocol_buff[4]>>8;
		cDisplayValueBuffer[0] = protocol_buff[4]&0x00FF;
		                                       
		cFlowSetpointBuffer[3] = protocol_buff[5]>>8;
		cFlowSetpointBuffer[2] = protocol_buff[5]&0x00FF;
		cFlowSetpointBuffer[1] = protocol_buff[6]>>8;
		cFlowSetpointBuffer[0] = protocol_buff[6]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            *floatDisplayValue,  // 30004   Real    Display Value   
            *floatFlowSetpoint,  // 30006   Real    Flow Setpoint
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0);
	return 0;
}

uint16_t MEECO_DataColumnsNumber()
{
	return 2;
}

