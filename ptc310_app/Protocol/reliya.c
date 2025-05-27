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
	Instrument_USART_SetRcvMode(USART_RCV_DELAY,20,5);
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
		
        pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
        pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
	}

	return 0;
	
err_exit:
	return 1;
}

/*
	RELIYA：HGPC-100
	寄存器	类型	功能描述	数值
	30001	Int	Communication Fault	0：Normal；1：Fault
	30002	Real	Counter 0.1 / Probe 1	
	30004	Real	Counter 0.2 / Probe 1	
	30006	Real	Counter 0.3 / Probe 1	
	30008	Real	Counter 0.5 / Probe 1		
 */
uint16_t RELIYA_HGPC_100_DataOutput(char* strOutput)
{
    char cCounterZeroPointOneBuffer[4] = {0};
    char cCounterZeroPointTwoBuffer[4] = {0};
    char cCounterZeroPointThreeBuffer[4] = {0};
    char cCounterZeroPointFiveBuffer[4] = {0};
	
	float * floatCounterZeroPointOneBuffer   = (float *)(cCounterZeroPointOneBuffer);
	float * floatCounterZeroPointTwoBuffer   = (float *)(cCounterZeroPointTwoBuffer);
	float * floatCounterZeroPointThreeBuffer = (float *)(cCounterZeroPointThreeBuffer);
	float * floatCounterZeroPointFiveBuffer  = (float *)(cCounterZeroPointFiveBuffer);
		
    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cCounterZeroPointOneBuffer[3] = protocol_buff[2]>>8;
		cCounterZeroPointOneBuffer[2] = protocol_buff[2]&0x00FF;
		cCounterZeroPointOneBuffer[1] = protocol_buff[1]>>8;
		cCounterZeroPointOneBuffer[0] = protocol_buff[1]&0x00FF;
		
		cCounterZeroPointTwoBuffer[3] = protocol_buff[4]>>8;
		cCounterZeroPointTwoBuffer[2] = protocol_buff[4]&0x00FF;
		cCounterZeroPointTwoBuffer[1] = protocol_buff[3]>>8;
		cCounterZeroPointTwoBuffer[0] = protocol_buff[3]&0x00FF;
        
		cCounterZeroPointThreeBuffer[3] = protocol_buff[6]>>8;
		cCounterZeroPointThreeBuffer[2] = protocol_buff[6]&0x00FF;
		cCounterZeroPointThreeBuffer[1] = protocol_buff[5]>>8;
		cCounterZeroPointThreeBuffer[0] = protocol_buff[5]&0x00FF;
        
		cCounterZeroPointFiveBuffer[3] = protocol_buff[8]>>8;
		cCounterZeroPointFiveBuffer[2] = protocol_buff[8]&0x00FF;
		cCounterZeroPointFiveBuffer[1] = protocol_buff[7]>>8;
		cCounterZeroPointFiveBuffer[0] = protocol_buff[7]&0x00FF;
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cCounterZeroPointOneBuffer[3] = protocol_buff[1]>>8;
		cCounterZeroPointOneBuffer[2] = protocol_buff[1]&0x00FF;
		cCounterZeroPointOneBuffer[1] = protocol_buff[2]>>8;
		cCounterZeroPointOneBuffer[0] = protocol_buff[2]&0x00FF;
		
		cCounterZeroPointTwoBuffer[3] = protocol_buff[3]>>8;
		cCounterZeroPointTwoBuffer[2] = protocol_buff[3]&0x00FF;
		cCounterZeroPointTwoBuffer[1] = protocol_buff[4]>>8;
		cCounterZeroPointTwoBuffer[0] = protocol_buff[4]&0x00FF;
        
		cCounterZeroPointThreeBuffer[3] = protocol_buff[5]>>8;
		cCounterZeroPointThreeBuffer[2] = protocol_buff[5]&0x00FF;
		cCounterZeroPointThreeBuffer[1] = protocol_buff[6]>>8;
		cCounterZeroPointThreeBuffer[0] = protocol_buff[6]&0x00FF;
        
		cCounterZeroPointFiveBuffer[3] = protocol_buff[7]>>8;
		cCounterZeroPointFiveBuffer[2] = protocol_buff[7]&0x00FF;
		cCounterZeroPointFiveBuffer[1] = protocol_buff[8]>>8;
		cCounterZeroPointFiveBuffer[0] = protocol_buff[8]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁

	// printf("DELTAF_DataOutput::cO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	// 	cO2ppbBuffer[0], cO2ppbBuffer[1], cO2ppbBuffer[2], cO2ppbBuffer[3]);
	// printf("DELTAF_DataOutput::cHO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	// 	cHO2ppbBuffer[0], cHO2ppbBuffer[1], cHO2ppbBuffer[2], cHO2ppbBuffer[3]);
	
    sprintf(strOutput, "%f,%f,%f,%f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
          *floatCounterZeroPointOneBuffer,     // 30002	Real	Counter 0.1 / Probe 1
          *floatCounterZeroPointTwoBuffer,     // 30004	Real	Counter 0.2 / Probe 1
          *floatCounterZeroPointThreeBuffer,   // 30006	Real	Counter 0.3 / Probe 1
          *floatCounterZeroPointFiveBuffer,    // 30008	Real	Counter 0.5 / Probe 1
          0.0, 0.0, 0.0, 
          0.0, 0.0, 0.0);
	return 0;
}

uint16_t RELIYA_HGPC_100_DataColumnsNumber()
{
	return 4;
}

