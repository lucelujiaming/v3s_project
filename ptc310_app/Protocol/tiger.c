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
    Instrument_USART_SetRcvMode(USART_RCV_DELAY,50,10);
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
			
			pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
        	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁

			err= 0;
		}
		break;
	case 1:
		if(*(serial_buff)>= '0' && *(serial_buff)<= '3')
		{
        	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
			protocol_buff[5]= *(serial_buff)- '0';
        	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
        	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
			strncpy((char*)&protocol_buff[1],(const char*)(serial_buff),str_size);
        	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			err= 0;
		}
		break;
	default:
		break;
	}

	return err;
}


/*
    TIGER： HALO，HALO 3Q，SPARK
    Register   Type    Description
    30001      Int     Communication Fault
    30002      Int     Name
    30006      Int     Operating Mode
    30007      Real    Concertration(ppb)
 */
uint16_t TIGER_DataOutput(char* strOutput)
{
	char cConcertrationBuffer[4] = {0};
	float * floatConcertration  = (float *)cConcertrationBuffer;

    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cConcertrationBuffer[3] = protocol_buff[7]>>8;
		cConcertrationBuffer[2] = protocol_buff[7]&0x00FF;
		cConcertrationBuffer[1] = protocol_buff[6]>>8;
		cConcertrationBuffer[0] = protocol_buff[6]&0x00FF;
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cConcertrationBuffer[3] = protocol_buff[6]>>8;
		cConcertrationBuffer[2] = protocol_buff[6]&0x00FF;
		cConcertrationBuffer[1] = protocol_buff[7]>>8;
		cConcertrationBuffer[0] = protocol_buff[7]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            *floatConcertration, // Real    Concertration(ppb)
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0);
	return 0;
}

uint16_t TIGER_DataColumnsNumber()
{
	return 1;
}

