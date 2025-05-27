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
	Instrument_USART_SetRcvMode(USART_RCV_CHAR, 0x02, 0x03);
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
		
		ptr= eptr+ 2;
	}

	return 0;
}

/*
    HCTM：WCPC-0703E
	寄存器	类型	功能描述
	30001	Int Communication Fault
	30002	Real	Particle / SEC
	30004	Real	Particle / cm3
	30006	Real	Total Particle

 */
uint16_t HCTM_WCPC0703E_DataOutput(char* strOutput)
{
	char cParticleSECBuffer[4] = {0};
	char cParticleCM3Buffer[4] = {0};
	char cTotalParticleBuffer[4] = {0};
	float * floatParticleSEC    = (float *)cParticleSECBuffer;
	float * floatParticleCM3    = (float *)cParticleCM3Buffer;
	float * floatTotalParticle  = (float *)cTotalParticleBuffer;
	
	pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cParticleSECBuffer[3] = protocol_buff[2]>>8;
		cParticleSECBuffer[2] = protocol_buff[2]&0x00FF;
		cParticleSECBuffer[1] = protocol_buff[1]>>8;
		cParticleSECBuffer[0] = protocol_buff[1]&0x00FF;
        
		cParticleCM3Buffer[3] = protocol_buff[4]>>8;
		cParticleCM3Buffer[2] = protocol_buff[4]&0x00FF;
		cParticleCM3Buffer[1] = protocol_buff[3]>>8;
		cParticleCM3Buffer[0] = protocol_buff[3]&0x00FF;
        
		cTotalParticleBuffer[3] = protocol_buff[6]>>8;
		cTotalParticleBuffer[2] = protocol_buff[6]&0x00FF;
		cTotalParticleBuffer[1] = protocol_buff[5]>>8;
		cTotalParticleBuffer[0] = protocol_buff[5]&0x00FF;
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cParticleSECBuffer[3] = protocol_buff[1]>>8;
		cParticleSECBuffer[2] = protocol_buff[1]&0x00FF;
		cParticleSECBuffer[1] = protocol_buff[2]>>8;
		cParticleSECBuffer[0] = protocol_buff[2]&0x00FF;
        
		cParticleCM3Buffer[3] = protocol_buff[3]>>8;
		cParticleCM3Buffer[2] = protocol_buff[3]&0x00FF;
		cParticleCM3Buffer[1] = protocol_buff[4]>>8;
		cParticleCM3Buffer[0] = protocol_buff[4]&0x00FF;
        
		cTotalParticleBuffer[3] = protocol_buff[5]>>8;
		cTotalParticleBuffer[2] = protocol_buff[5]&0x00FF;
		cTotalParticleBuffer[1] = protocol_buff[6]>>8;
		cTotalParticleBuffer[0] = protocol_buff[6]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
            *floatParticleSEC,     // 30002	Real	Particle / SEC
            *floatParticleCM3,     // 30004	Real	Particle / cm3
            *floatTotalParticle,   // 30006	Real	Total Particle
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0);
	return 0;
}

uint16_t HCTM_WCPC0703E_DataColumnsNumber()
{
	return 3;
}

