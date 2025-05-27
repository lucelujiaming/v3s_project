//delta_f.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "delta_f.h"

#define DELTAF_CMD_MAX		2

static uint8_t cmd_cnt;

char DELTAF_Cmd[DELTAF_CMD_MAX][8]= 
{
	{0x01,0x01,0x00,0x00,0x00,0x01,0x0D},
	{0x01,0x01,0x01,0x00,0x00,0x02,0x0D},
//	{0x01,0x01,0x66,0x00,0x00,0x67,0x0D}
};

uint8_t deltaf_check_sum(uint8_t len)
{
	uint8_t i;
	uint16_t sum= 0;
	
	for(i= 0;i< len - 4;i++)
	{
		sum+= *(serial_buff + i + 1);
	}
	
	if(sum== _get_int16_int8_big_endian(serial_buff + len - 3))
	{
		return 1;
	}
	
	return 0;
}

void DELTAF_Init(int fd, uint16_t addr)
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

uint16_t DELTAF_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= DELTAF_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= 7;
	memmove(serial_buff,(const char*)DELTAF_Cmd[cmd_cnt],len);
	
	return len;
}

uint8_t DELTAF_Analysis(uint16_t len)
{
	uint8_t err;
	
	err= 1;
	
	if(deltaf_check_sum(len))
	{
		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
		switch(*(serial_buff + 2))
		{
		case 0x00:
			protocol_buff[1]= *(serial_buff+4);
			protocol_buff[2]= *(serial_buff+5);
			protocol_buff[3]= *(serial_buff+6);
			break;
		case 0x01:
			if(little_endian)
			{
				protocol_buff[5]= _get_int16_int8_big_endian(serial_buff+4);
				protocol_buff[4]= _get_int16_int8_big_endian(serial_buff+6);
				// printf("DELTAF_Analysis::little_endian\r\n");
			}
			else
			{
				protocol_buff[4]= _get_int16_int8_big_endian(serial_buff+4);
				protocol_buff[5]= _get_int16_int8_big_endian(serial_buff+6); 
				// printf("DELTAF_Analysis::big_endian\r\n");
			}
			break;
		case 0x66:
			if(little_endian)
			{
				protocol_buff[7]= _get_int16_int8_big_endian(serial_buff+7);
				protocol_buff[6]= _get_int16_int8_big_endian(serial_buff+9); 
			}
			else
			{
				protocol_buff[6]= _get_int16_int8_big_endian(serial_buff+7);
				protocol_buff[7]= _get_int16_int8_big_endian(serial_buff+9); 
			}
			break;
		default:
			break;
		}
    	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
		err= 0;
	}
	else
	{
		printf("deltaf_check_sum error with %d\n", len);
	}

	return err;
}

/*
   Delta F：DF310E，DF550E，DF560，DF745，DF750
   寄存器      类型      功能描述	            数值
     30001	Int	    Communication Fault	0：Normal；1：Fault
     30002	Int	    STATUS 0	
     30003	Int	    STATUS 1	
     30004	Int	    STATUS 2	
     30005	Real	O2 ppb	
     30007	Real	H2O ppb	            (760E才有）
 */
uint16_t DELTAF_DataOutput(char* strOutput)
{
	char cO2ppbBuffer[4] = {0};
	char cHO2ppbBuffer[4] = {0};
	float * floatO2ppb  = (float *)cO2ppbBuffer;
	float * floatHO2ppb = (float *)cHO2ppbBuffer;
	
	pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cO2ppbBuffer[3] = protocol_buff[5]>>8;
		cO2ppbBuffer[2] = protocol_buff[5]&0x00FF;
		cO2ppbBuffer[1] = protocol_buff[4]>>8;
		cO2ppbBuffer[0] = protocol_buff[4]&0x00FF;
		
		cHO2ppbBuffer[3] = protocol_buff[7]>>8;
		cHO2ppbBuffer[2] = protocol_buff[7]&0x00FF;
		cHO2ppbBuffer[1] = protocol_buff[6]>>8;
		cHO2ppbBuffer[0] = protocol_buff[6]&0x00FF;
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cO2ppbBuffer[3] = protocol_buff[4]>>8;
		cO2ppbBuffer[2] = protocol_buff[4]&0x00FF;
		cO2ppbBuffer[1] = protocol_buff[5]>>8;
		cO2ppbBuffer[0] = protocol_buff[5]&0x00FF;
		
		cHO2ppbBuffer[3] = protocol_buff[6]>>8;
		cHO2ppbBuffer[2] = protocol_buff[6]&0x00FF;
		cHO2ppbBuffer[1] = protocol_buff[7]>>8;
		cHO2ppbBuffer[0] = protocol_buff[7]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
	
//	// PPM -> PPB
//	if(*floatO2ppb < 0.001)
//	{
//		*floatO2ppb = *floatO2ppb * 1000;
//	}
//	// PPB -> PPT
//	if(*floatO2ppb < 0.001)
//	{
//		*floatO2ppb = *floatO2ppb * 1000;
//	}
//	
//	// PPM -> PPB
//	if(*floatHO2ppb < 0.001)
//	{
//		*floatHO2ppb = *floatHO2ppb * 1000;
//	}
//	// PPB -> PPT
//	if(*floatHO2ppb < 0.001)
//	{
//		*floatHO2ppb = *floatHO2ppb * 1000;
//	}

	// printf("DELTAF_DataOutput::cO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	//	cO2ppbBuffer[0], cO2ppbBuffer[1], cO2ppbBuffer[2], cO2ppbBuffer[3]);
	// printf("DELTAF_DataOutput::cHO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	//	cHO2ppbBuffer[0], cHO2ppbBuffer[1], cHO2ppbBuffer[2], cHO2ppbBuffer[3]);
	
    sprintf(strOutput, "%f,%f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            *floatO2ppb,       // 30005	Real	O2 ppb	
            *floatHO2ppb,      // 30007	Real	H2O ppb	  
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0);
	return 0;
}

uint16_t DELTAF_DataColumnsNumber()
{
	return 2;
}

