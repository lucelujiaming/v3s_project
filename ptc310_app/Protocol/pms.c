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
	Instrument_USART_SetRcvMode(USART_RCV_CHAR, 0x02, 0x03);
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
			
			pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
			
			pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			
			pos+= 10;
		}
				
		// Get AI Value
		for(i= 0;i<6;i++)
		{
			memset(data_temp,0,20);
			strncpy(data_temp,(const char*)(serial_buff + pos),10);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
			pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			pos+= 10;
		}
		
		err= 0;
	}
	
	return err;
}


/*
	寄存器	类型	功能描述
	30001	Int	Communication Fault
	30002	Real	Counter 0.1 / Probe 1
	30004	Real	Counter 0.2 / Probe 1
	30006	Real	Counter 0.3 / Probe 1
	30008	Real	Counter 0.5 / Probe 1
	30010	Real	Counter 1 / Probe 1
	30012	Real	Counter 2 / Probe 1
	30014	Real	Counter 3 / Probe 1
	30016	Real	Counter 5 / Probe 1
	30018	Real	Laser Ref. / Probe 1
	30052	Real	Counter 0.1 / Probe 2
	30054	Real	Counter 0.2 / Probe 2
	30056	Real	Counter 0.3 / Probe 2
	30058	Real	Counter 0.5 / Probe 2
	30060	Real	Counter 1 / Probe 2
	30062	Real	Counter 2 / Probe 2
	30064	Real	Counter 3 / Probe 2
	30066	Real	Counter 5 / Probe 2
	30068	Real	Laser Ref./ Probe 2
 */
// 目前只输出Probe 1的内容。
uint16_t PMS_PDS_PA_DataOutput(char* strOutput)
{
    char cCounterZeroPointOneBuffer[4] = {0};
    char cCounterZeroPointTwoBuffer[4] = {0};
    char cCounterZeroPointThreeBuffer[4] = {0};
    char cCounterZeroPointFiveBuffer[4] = {0};
    
    char cCounterOneBuffer[4] = {0};
    char cCounterTwoBuffer[4] = {0};
    char cCounterThreeBuffer[4] = {0};
    char cCounterFiveBuffer[4] = {0};
    
    char cLaserRefBuffer[4] = {0};

	
	float * floatCounterZeroPointOneBufferPtr   = (float *)(cCounterZeroPointOneBuffer);
	float * floatCounterZeroPointTwoBufferPtr   = (float *)(cCounterZeroPointTwoBuffer);
	float * floatCounterZeroPointThreeBufferPtr = (float *)(cCounterZeroPointThreeBuffer);
	float * floatCounterZeroPointFiveBufferPtr  = (float *)(cCounterZeroPointFiveBuffer);
    
	float * floatCounterOneBufferPtr   = (float *)(cCounterOneBuffer);
	float * floatCounterTwoBufferPtr   = (float *)(cCounterTwoBuffer);
	float * floatCounterThreeBufferPtr = (float *)(cCounterThreeBuffer);
	float * floatCounterFiveBufferPtr  = (float *)(cCounterFiveBuffer);
    
	float * floatLaserRefBufferPtr = (float *)(cLaserRefBuffer);

    float floatCounterZeroPointOne[2],   floatCounterZeroPointTwo[2], 
          floatCounterZeroPointThree[2], floatCounterZeroPointFive[2],
          floatCounterOne[2],            floatCounterTwo[2], 
          floatCounterThree[2],          floatCounterFive[2],
          floatLaserRef[2];

    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁 
	if(little_endian)
	{
		cCounterZeroPointOneBuffer[3] = protocol_buff[2]>>8;
		cCounterZeroPointOneBuffer[2] = protocol_buff[2]&0x00FF;
		cCounterZeroPointOneBuffer[1] = protocol_buff[1]>>8;
		cCounterZeroPointOneBuffer[0] = protocol_buff[1]&0x00FF;
        floatCounterZeroPointOne[0] = *floatCounterZeroPointOneBufferPtr;
		
		cCounterZeroPointTwoBuffer[3] = protocol_buff[4]>>8;
		cCounterZeroPointTwoBuffer[2] = protocol_buff[4]&0x00FF;
		cCounterZeroPointTwoBuffer[1] = protocol_buff[3]>>8;
		cCounterZeroPointTwoBuffer[0] = protocol_buff[3]&0x00FF;
        floatCounterZeroPointTwo[0] = *floatCounterZeroPointTwoBufferPtr;
        
		cCounterZeroPointThreeBuffer[3] = protocol_buff[6]>>8;
		cCounterZeroPointThreeBuffer[2] = protocol_buff[6]&0x00FF;
		cCounterZeroPointThreeBuffer[1] = protocol_buff[5]>>8;
		cCounterZeroPointThreeBuffer[0] = protocol_buff[5]&0x00FF;
        floatCounterZeroPointThree[0] = *floatCounterZeroPointThreeBufferPtr;
        
		cCounterZeroPointFiveBuffer[3] = protocol_buff[8]>>8;
		cCounterZeroPointFiveBuffer[2] = protocol_buff[8]&0x00FF;
		cCounterZeroPointFiveBuffer[1] = protocol_buff[7]>>8;
		cCounterZeroPointFiveBuffer[0] = protocol_buff[7]&0x00FF;
        floatCounterZeroPointFive[0] = *floatCounterZeroPointFiveBufferPtr;
        
		cCounterOneBuffer[3] = protocol_buff[10]>>8;
		cCounterOneBuffer[2] = protocol_buff[10]&0x00FF;
		cCounterOneBuffer[1] = protocol_buff[9]>>8;
		cCounterOneBuffer[0] = protocol_buff[9]&0x00FF;
        floatCounterOne[0] = *floatCounterOneBufferPtr;
        
		cCounterTwoBuffer[3] = protocol_buff[12]>>8;
		cCounterTwoBuffer[2] = protocol_buff[12]&0x00FF;
		cCounterTwoBuffer[1] = protocol_buff[11]>>8;
		cCounterTwoBuffer[0] = protocol_buff[11]&0x00FF;
        floatCounterTwo[0] = *floatCounterTwoBufferPtr;
        
		cCounterThreeBuffer[3] = protocol_buff[14]>>8;
		cCounterThreeBuffer[2] = protocol_buff[14]&0x00FF;
		cCounterThreeBuffer[1] = protocol_buff[13]>>8;
		cCounterThreeBuffer[0] = protocol_buff[13]&0x00FF;
        floatCounterThree[0] = *floatCounterThreeBufferPtr;
        
		cCounterFiveBuffer[3] = protocol_buff[16]>>8;
		cCounterFiveBuffer[2] = protocol_buff[16]&0x00FF;
		cCounterFiveBuffer[1] = protocol_buff[15]>>8;
		cCounterFiveBuffer[0] = protocol_buff[15]&0x00FF;
        floatCounterFive[0] = *floatCounterFiveBufferPtr;
        
		cLaserRefBuffer[3] = protocol_buff[18]>>8;
		cLaserRefBuffer[2] = protocol_buff[18]&0x00FF;
		cLaserRefBuffer[1] = protocol_buff[17]>>8;
		cLaserRefBuffer[0] = protocol_buff[17]&0x00FF;
        floatLaserRef[0] = *floatLaserRefBufferPtr;
		//		printf("DELTAF_DataOutput::little_endian\r\n");

		cCounterZeroPointOneBuffer[3] = protocol_buff[52]>>8;
		cCounterZeroPointOneBuffer[2] = protocol_buff[52]&0x00FF;
		cCounterZeroPointOneBuffer[1] = protocol_buff[51]>>8;
		cCounterZeroPointOneBuffer[0] = protocol_buff[51]&0x00FF;
        floatCounterZeroPointOne[1] = *floatCounterZeroPointOneBufferPtr;
		
		cCounterZeroPointTwoBuffer[3] = protocol_buff[54]>>8;
		cCounterZeroPointTwoBuffer[2] = protocol_buff[54]&0x00FF;
		cCounterZeroPointTwoBuffer[1] = protocol_buff[53]>>8;
		cCounterZeroPointTwoBuffer[0] = protocol_buff[53]&0x00FF;
        floatCounterZeroPointTwo[1] = *floatCounterZeroPointTwoBufferPtr;
        
		cCounterZeroPointThreeBuffer[3] = protocol_buff[56]>>8;
		cCounterZeroPointThreeBuffer[2] = protocol_buff[56]&0x00FF;
		cCounterZeroPointThreeBuffer[1] = protocol_buff[55]>>8;
		cCounterZeroPointThreeBuffer[0] = protocol_buff[55]&0x00FF;
        floatCounterZeroPointThree[1] = *floatCounterZeroPointThreeBufferPtr;
        
		cCounterZeroPointFiveBuffer[3] = protocol_buff[58]>>8;
		cCounterZeroPointFiveBuffer[2] = protocol_buff[58]&0x00FF;
		cCounterZeroPointFiveBuffer[1] = protocol_buff[57]>>8;
		cCounterZeroPointFiveBuffer[0] = protocol_buff[57]&0x00FF;
        floatCounterZeroPointFive[1] = *floatCounterZeroPointFiveBufferPtr;
        
		cCounterOneBuffer[3] = protocol_buff[60]>>8;
		cCounterOneBuffer[2] = protocol_buff[60]&0x00FF;
		cCounterOneBuffer[1] = protocol_buff[59]>>8;
		cCounterOneBuffer[0] = protocol_buff[59]&0x00FF;
        floatCounterOne[1] = *floatCounterOneBufferPtr;
        
		cCounterTwoBuffer[3] = protocol_buff[62]>>8;
		cCounterTwoBuffer[2] = protocol_buff[62]&0x00FF;
		cCounterTwoBuffer[1] = protocol_buff[61]>>8;
		cCounterTwoBuffer[0] = protocol_buff[61]&0x00FF;
        floatCounterTwo[1] = *floatCounterTwoBufferPtr;
        
		cCounterThreeBuffer[3] = protocol_buff[64]>>8;
		cCounterThreeBuffer[2] = protocol_buff[64]&0x00FF;
		cCounterThreeBuffer[1] = protocol_buff[63]>>8;
		cCounterThreeBuffer[0] = protocol_buff[63]&0x00FF;
        floatCounterThree[1] = *floatCounterThreeBufferPtr;
        
		cCounterFiveBuffer[3] = protocol_buff[66]>>8;
		cCounterFiveBuffer[2] = protocol_buff[66]&0x00FF;
		cCounterFiveBuffer[1] = protocol_buff[65]>>8;
		cCounterFiveBuffer[0] = protocol_buff[65]&0x00FF;
        floatCounterFive[1] = *floatCounterFiveBufferPtr;
        
		cLaserRefBuffer[3] = protocol_buff[68]>>8;
		cLaserRefBuffer[2] = protocol_buff[68]&0x00FF;
		cLaserRefBuffer[1] = protocol_buff[67]>>8;
		cLaserRefBuffer[0] = protocol_buff[67]&0x00FF;
        floatLaserRef[1] = *floatLaserRefBufferPtr;
        
	}
	else
	{
		cCounterZeroPointOneBuffer[3] = protocol_buff[1]>>8;
		cCounterZeroPointOneBuffer[2] = protocol_buff[1]&0x00FF;
		cCounterZeroPointOneBuffer[1] = protocol_buff[2]>>8;
		cCounterZeroPointOneBuffer[0] = protocol_buff[2]&0x00FF;
        floatCounterZeroPointOne[0] = *floatCounterZeroPointOneBufferPtr;
		
		cCounterZeroPointTwoBuffer[3] = protocol_buff[3]>>8;
		cCounterZeroPointTwoBuffer[2] = protocol_buff[3]&0x00FF;
		cCounterZeroPointTwoBuffer[1] = protocol_buff[4]>>8;
		cCounterZeroPointTwoBuffer[0] = protocol_buff[4]&0x00FF;
        floatCounterZeroPointTwo[0] = *floatCounterZeroPointTwoBufferPtr;
        
		cCounterZeroPointThreeBuffer[3] = protocol_buff[5]>>8;
		cCounterZeroPointThreeBuffer[2] = protocol_buff[5]&0x00FF;
		cCounterZeroPointThreeBuffer[1] = protocol_buff[6]>>8;
		cCounterZeroPointThreeBuffer[0] = protocol_buff[6]&0x00FF;
        floatCounterZeroPointThree[0] = *floatCounterZeroPointThreeBufferPtr;
        
		cCounterZeroPointFiveBuffer[3] = protocol_buff[7]>>8;
		cCounterZeroPointFiveBuffer[2] = protocol_buff[7]&0x00FF;
		cCounterZeroPointFiveBuffer[1] = protocol_buff[8]>>8;
		cCounterZeroPointFiveBuffer[0] = protocol_buff[8]&0x00FF;
        floatCounterZeroPointFive[0] = *floatCounterZeroPointFiveBufferPtr;
        
		cCounterOneBuffer[3] = protocol_buff[9]>>8;
		cCounterOneBuffer[2] = protocol_buff[9]&0x00FF;
		cCounterOneBuffer[1] = protocol_buff[10]>>8;
		cCounterOneBuffer[0] = protocol_buff[10]&0x00FF;
        floatCounterOne[0] = *floatCounterOneBufferPtr;
        
		cCounterTwoBuffer[3] = protocol_buff[11]>>8;
		cCounterTwoBuffer[2] = protocol_buff[11]&0x00FF;
		cCounterTwoBuffer[1] = protocol_buff[12]>>8;
		cCounterTwoBuffer[0] = protocol_buff[12]&0x00FF;
        floatCounterTwo[0] = *floatCounterTwoBufferPtr;
        
		cCounterThreeBuffer[3] = protocol_buff[13]>>8;
		cCounterThreeBuffer[2] = protocol_buff[13]&0x00FF;
		cCounterThreeBuffer[1] = protocol_buff[14]>>8;
		cCounterThreeBuffer[0] = protocol_buff[14]&0x00FF;
        floatCounterThree[0] = *floatCounterThreeBufferPtr;
        
		cCounterFiveBuffer[3] = protocol_buff[15]>>8;
		cCounterFiveBuffer[2] = protocol_buff[15]&0x00FF;
		cCounterFiveBuffer[1] = protocol_buff[16]>>8;
		cCounterFiveBuffer[0] = protocol_buff[16]&0x00FF;
        floatCounterFive[0] = *floatCounterFiveBufferPtr;
        
		cLaserRefBuffer[3] = protocol_buff[17]>>8;
		cLaserRefBuffer[2] = protocol_buff[17]&0x00FF;
		cLaserRefBuffer[1] = protocol_buff[18]>>8;
		cLaserRefBuffer[0] = protocol_buff[18]&0x00FF;
        floatLaserRef[0] = *floatLaserRefBufferPtr;
		//		printf("DELTAF_DataOutput::big_endian\r\n");

		cCounterZeroPointOneBuffer[3] = protocol_buff[51]>>8;
		cCounterZeroPointOneBuffer[2] = protocol_buff[51]&0x00FF;
		cCounterZeroPointOneBuffer[1] = protocol_buff[52]>>8;
		cCounterZeroPointOneBuffer[0] = protocol_buff[52]&0x00FF;
        floatCounterZeroPointOne[1] = *floatCounterZeroPointOneBufferPtr;
		
		cCounterZeroPointTwoBuffer[3] = protocol_buff[53]>>8;
		cCounterZeroPointTwoBuffer[2] = protocol_buff[53]&0x00FF;
		cCounterZeroPointTwoBuffer[1] = protocol_buff[54]>>8;
		cCounterZeroPointTwoBuffer[0] = protocol_buff[54]&0x00FF;
        floatCounterZeroPointTwo[1] = *floatCounterZeroPointTwoBufferPtr;
        
		cCounterZeroPointThreeBuffer[3] = protocol_buff[55]>>8;
		cCounterZeroPointThreeBuffer[2] = protocol_buff[55]&0x00FF;
		cCounterZeroPointThreeBuffer[1] = protocol_buff[56]>>8;
		cCounterZeroPointThreeBuffer[0] = protocol_buff[56]&0x00FF;
        floatCounterZeroPointThree[1] = *floatCounterZeroPointThreeBufferPtr;
        
		cCounterZeroPointFiveBuffer[3] = protocol_buff[57]>>8;
		cCounterZeroPointFiveBuffer[2] = protocol_buff[57]&0x00FF;
		cCounterZeroPointFiveBuffer[1] = protocol_buff[58]>>8;
		cCounterZeroPointFiveBuffer[0] = protocol_buff[58]&0x00FF;
        floatCounterZeroPointFive[1] = *floatCounterZeroPointFiveBufferPtr;
        
		cCounterOneBuffer[3] = protocol_buff[59]>>8;
		cCounterOneBuffer[2] = protocol_buff[59]&0x00FF;
		cCounterOneBuffer[1] = protocol_buff[60]>>8;
		cCounterOneBuffer[0] = protocol_buff[60]&0x00FF;
        floatCounterOne[1] = *floatCounterOneBufferPtr;
        
		cCounterTwoBuffer[3] = protocol_buff[61]>>8;
		cCounterTwoBuffer[2] = protocol_buff[61]&0x00FF;
		cCounterTwoBuffer[1] = protocol_buff[62]>>8;
		cCounterTwoBuffer[0] = protocol_buff[62]&0x00FF;
        floatCounterTwo[1] = *floatCounterTwoBufferPtr;
        
		cCounterThreeBuffer[3] = protocol_buff[63]>>8;
		cCounterThreeBuffer[2] = protocol_buff[63]&0x00FF;
		cCounterThreeBuffer[1] = protocol_buff[64]>>8;
		cCounterThreeBuffer[0] = protocol_buff[64]&0x00FF;
        floatCounterThree[1] = *floatCounterThreeBufferPtr;
        
		cCounterFiveBuffer[3] = protocol_buff[65]>>8;
		cCounterFiveBuffer[2] = protocol_buff[65]&0x00FF;
		cCounterFiveBuffer[1] = protocol_buff[66]>>8;
		cCounterFiveBuffer[0] = protocol_buff[66]&0x00FF;
        floatCounterFive[1] = *floatCounterFiveBufferPtr;
        
		cLaserRefBuffer[3] = protocol_buff[67]>>8;
		cLaserRefBuffer[2] = protocol_buff[67]&0x00FF;
		cLaserRefBuffer[1] = protocol_buff[68]>>8;
		cLaserRefBuffer[0] = protocol_buff[68]&0x00FF;
        floatLaserRef[1] = *floatLaserRefBufferPtr;
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁

	// printf("DELTAF_DataOutput::cO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	// 	cO2ppbBuffer[0], cO2ppbBuffer[1], cO2ppbBuffer[2], cO2ppbBuffer[3]);
	// printf("DELTAF_DataOutput::cHO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	// 	cHO2ppbBuffer[0], cHO2ppbBuffer[1], cHO2ppbBuffer[2], cHO2ppbBuffer[3]);
	
    sprintf(strOutput, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%.1f",
          floatCounterZeroPointOne[0],     // 30002	Real	Counter 0.1 / Probe 1
          floatCounterZeroPointTwo[0],     // 30004	Real	Counter 0.2 / Probe 1
          floatCounterZeroPointThree[0],   // 30006	Real	Counter 0.3 / Probe 1
          floatCounterZeroPointFive[0],    // 30008	Real	Counter 0.5 / Probe 1
          floatCounterOne[0],              // 30010	Real	Counter 1 / Probe 1
          floatCounterTwo[0],              // 30012	Real	Counter 2 / Probe 1
          floatCounterThree[0],            // 30014	Real	Counter 3 / Probe 1
          floatCounterFive[0],             // 30016	Real	Counter 5 / Probe 1
          floatLaserRef[0],                // 30018	Real	Laser Ref. / Probe 1
          0.0);
	return 0;
}

uint16_t PMS_PDS_PA_DataColumnsNumber()
{
    return 9;
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
			pos+= 10;
		}
		
		pos+= 2;
		
		for(i= 0;i<7;i++)
		{
			memset(data_temp,0,20);
			strncpy(data_temp,(const char*)(serial_buff + pos),10);
			
			f_value= atof(data_temp);
			u32_value= real_to_u32(f_value);
			
            pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
			pos+= 10;
		}
		
		err= 0;
	}
	
	return err;
}

uint16_t PMS_HPGP_101_DataOutput(char* strOutput)
{
	return PMS_PDS_PA_DataOutput(strOutput);
}

uint16_t PMS_HPGP_101_DataColumnsNumber()
{
	return PMS_PDS_PA_DataColumnsNumber();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// PMS_Init_2 for Lasair III
void PMS_Init_2(int fd, uint16_t addr)
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
	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
	if(*ptr== '0')
	{
		protocol_buff[15]= 0;
	}
	else
	{
		protocol_buff[15]= 1;
	}
	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
			
            pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
						
            			pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
							
            				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            				pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
								
            					pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            					pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
            						pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
            						pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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

/*
	Register	Type	Description
	30001	    Int	    Communication Fault
	30002	    Real	0.3
	30004	    Real	0.5
	30006	    Byte	1.0
	30008	    Byte	2.0
	30010	    Byte	5.0
	30012	    Byte	10.0
	30014	    Byte	25.0
	30016	    Int	    Ref Good
 */

uint16_t PMS_LASAIR_III_DataOutput(char* strOutput)
{
    char cZeroPointThreeBuffer[4];
    char cZeroPointFiveBuffer[4];
    char cOneBuffer[4];
    char cTwoBuffer[4];
    char cFiveBuffer[4];
    char cTenBuffer[4];
    char cTwenyFiveBuffer[4];
    // char cRefGoodBuffer[4];
    
	float * floatZeroPointThree  = (float *)cZeroPointThreeBuffer;
	float * floatZeroPointFive   = (float *)cZeroPointFiveBuffer;
	float * floatOne             = (float *)cOneBuffer;
	float * floatTwo             = (float *)cTwoBuffer;
	float * floatFive            = (float *)cFiveBuffer;
	float * floatTen             = (float *)cTenBuffer;
	float * floatTwenyFive       = (float *)cTwenyFiveBuffer;
	// int   * floatRefGood         = (int   *)cRefGoodBuffer;
    
    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cZeroPointThreeBuffer[3] = protocol_buff[2]>>8;
		cZeroPointThreeBuffer[2] = protocol_buff[2]&0x00FF;
		cZeroPointThreeBuffer[1] = protocol_buff[1]>>8;
		cZeroPointThreeBuffer[0] = protocol_buff[1]&0x00FF;
		
		cZeroPointFiveBuffer[3] = protocol_buff[4]>>8;
		cZeroPointFiveBuffer[2] = protocol_buff[4]&0x00FF;
		cZeroPointFiveBuffer[1] = protocol_buff[3]>>8;
		cZeroPointFiveBuffer[0] = protocol_buff[3]&0x00FF;
        
		cOneBuffer[3] = protocol_buff[6]>>8;
		cOneBuffer[2] = protocol_buff[6]&0x00FF;
		cOneBuffer[1] = protocol_buff[5]>>8;
		cOneBuffer[0] = protocol_buff[5]&0x00FF;
        
		cTwoBuffer[3] = protocol_buff[8]>>8;
		cTwoBuffer[2] = protocol_buff[8]&0x00FF;
		cTwoBuffer[1] = protocol_buff[7]>>8;
		cTwoBuffer[0] = protocol_buff[7]&0x00FF;
        
		cFiveBuffer[3] = protocol_buff[10]>>8;
		cFiveBuffer[2] = protocol_buff[10]&0x00FF;
		cFiveBuffer[1] = protocol_buff[9]>>8;
		cFiveBuffer[0] = protocol_buff[9]&0x00FF;
        
		cTenBuffer[3] = protocol_buff[12]>>8;
		cTenBuffer[2] = protocol_buff[12]&0x00FF;
		cTenBuffer[1] = protocol_buff[11]>>8;
		cTenBuffer[0] = protocol_buff[11]&0x00FF;
        
		cTwenyFiveBuffer[3] = protocol_buff[14]>>8;
		cTwenyFiveBuffer[2] = protocol_buff[14]&0x00FF;
		cTwenyFiveBuffer[1] = protocol_buff[13]>>8;
		cTwenyFiveBuffer[0] = protocol_buff[13]&0x00FF;
        
		// cRefGoodBuffer[3] = protocol_buff[16]>>8;
		// cRefGoodBuffer[2] = protocol_buff[16]&0x00FF;
		// cRefGoodBuffer[1] = protocol_buff[15]>>8;
		// cRefGoodBuffer[0] = protocol_buff[15]&0x00FF;
		
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cZeroPointThreeBuffer[3] = protocol_buff[1]>>8;
		cZeroPointThreeBuffer[2] = protocol_buff[1]&0x00FF;
		cZeroPointThreeBuffer[1] = protocol_buff[2]>>8;
		cZeroPointThreeBuffer[0] = protocol_buff[2]&0x00FF;
		
		cZeroPointFiveBuffer[3] = protocol_buff[3]>>8;
		cZeroPointFiveBuffer[2] = protocol_buff[3]&0x00FF;
		cZeroPointFiveBuffer[1] = protocol_buff[4]>>8;
		cZeroPointFiveBuffer[0] = protocol_buff[4]&0x00FF;
        
		cOneBuffer[3] = protocol_buff[5]>>8;
		cOneBuffer[2] = protocol_buff[5]&0x00FF;
		cOneBuffer[1] = protocol_buff[6]>>8;
		cOneBuffer[0] = protocol_buff[6]&0x00FF;
        
		cTwoBuffer[3] = protocol_buff[7]>>8;
		cTwoBuffer[2] = protocol_buff[7]&0x00FF;
		cTwoBuffer[1] = protocol_buff[8]>>8;
		cTwoBuffer[0] = protocol_buff[8]&0x00FF;
        
		cFiveBuffer[3] = protocol_buff[9]>>8;
		cFiveBuffer[2] = protocol_buff[9]&0x00FF;
		cFiveBuffer[1] = protocol_buff[10]>>8;
		cFiveBuffer[0] = protocol_buff[10]&0x00FF;
        
		cTenBuffer[3] = protocol_buff[11]>>8;
		cTenBuffer[2] = protocol_buff[11]&0x00FF;
		cTenBuffer[1] = protocol_buff[12]>>8;
		cTenBuffer[0] = protocol_buff[12]&0x00FF;
        
		cTwenyFiveBuffer[3] = protocol_buff[13]>>8;
		cTwenyFiveBuffer[2] = protocol_buff[13]&0x00FF;
		cTwenyFiveBuffer[1] = protocol_buff[14]>>8;
		cTwenyFiveBuffer[0] = protocol_buff[14]&0x00FF;
        
		// cRefGoodBuffer[3] = protocol_buff[15]>>8;
		// cRefGoodBuffer[2] = protocol_buff[15]&0x00FF;
		// cRefGoodBuffer[1] = protocol_buff[16]>>8;
		// cRefGoodBuffer[0] = protocol_buff[16]&0x00FF;
		
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%f,%f,%f,%f,%f,%.1f,%.1f,%.1f",
            *floatZeroPointThree, // 	30002	    Real	0.3
            *floatZeroPointFive,  // 	30004	    Real	0.5
            *floatOne,            // 	30006	    Byte	1.0
            *floatTwo,            // 	30008	    Byte	2.0
            *floatFive,           // 	30010	    Byte	5.0
            *floatTen,            // 	30012	    Byte	10.0  
            *floatTwenyFive,      // 	30014	    Byte	25.0
            0.0, 0.0, 0.0);
    
	return 0;
}

uint16_t PMS_LASAIR_III_DataColumnsNumber()
{
	return 7;
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

    pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
	
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
	PMS：PDS-E
	寄存器	类型	功能描述
	30001	Int Communication Fault
	30002	Real	Counter 0.1 / Probe 1	
	30004	Real	Counter 0.2 / Probe 1	
	30006	Real	Counter 0.3 / Probe 1	
	30008	Real	Counter 0.5 / Probe 1	
	30010	Real	Counter 1 / Probe 1 
	30012	Real	Counter 2 / Probe 1 
	30014	Real	Counter 3 / Probe 1 
	30016	Real	Counter 5 / Probe 1 
	30018	Real	Laser Ref. / Probe 1	
 */
uint16_t PMS_PDS_E_DataOutput(char* strOutput)
{
    char cCounterZeroPointOneBuffer[4] = {0};
    char cCounterZeroPointTwoBuffer[4] = {0};
    char cCounterZeroPointThreeBuffer[4] = {0};
    char cCounterZeroPointFiveBuffer[4] = {0};
    
    char cCounterOneBuffer[4] = {0};
    char cCounterTwoBuffer[4] = {0};
    char cCounterThreeBuffer[4] = {0};
    char cCounterFiveBuffer[4] = {0};
    
    char cLaserRefBuffer[4] = {0};

	float * floatCounterZeroPointOneBuffer   = (float *)(cCounterZeroPointOneBuffer);
	float * floatCounterZeroPointTwoBuffer   = (float *)(cCounterZeroPointTwoBuffer);
	float * floatCounterZeroPointThreeBuffer = (float *)(cCounterZeroPointThreeBuffer);
	float * floatCounterZeroPointFiveBuffer  = (float *)(cCounterZeroPointFiveBuffer);
    
	float * floatCounterOneBuffer   = (float *)(cCounterOneBuffer);
	float * floatCounterTwoBuffer   = (float *)(cCounterTwoBuffer);
	float * floatCounterThreeBuffer = (float *)(cCounterThreeBuffer);
	float * floatCounterFiveBuffer  = (float *)(cCounterFiveBuffer);
    
	float * floatLaserRefBuffer = (float *)(cLaserRefBuffer);
	
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
        
		cCounterOneBuffer[3] = protocol_buff[10]>>8;
		cCounterOneBuffer[2] = protocol_buff[10]&0x00FF;
		cCounterOneBuffer[1] = protocol_buff[9]>>8;
		cCounterOneBuffer[0] = protocol_buff[9]&0x00FF;
        
		cCounterTwoBuffer[3] = protocol_buff[12]>>8;
		cCounterTwoBuffer[2] = protocol_buff[12]&0x00FF;
		cCounterTwoBuffer[1] = protocol_buff[11]>>8;
		cCounterTwoBuffer[0] = protocol_buff[11]&0x00FF;
        
		cCounterThreeBuffer[3] = protocol_buff[14]>>8;
		cCounterThreeBuffer[2] = protocol_buff[14]&0x00FF;
		cCounterThreeBuffer[1] = protocol_buff[13]>>8;
		cCounterThreeBuffer[0] = protocol_buff[13]&0x00FF;
        
		cCounterFiveBuffer[3] = protocol_buff[16]>>8;
		cCounterFiveBuffer[2] = protocol_buff[16]&0x00FF;
		cCounterFiveBuffer[1] = protocol_buff[15]>>8;
		cCounterFiveBuffer[0] = protocol_buff[15]&0x00FF;
        
		cLaserRefBuffer[3] = protocol_buff[18]>>8;
		cLaserRefBuffer[2] = protocol_buff[18]&0x00FF;
		cLaserRefBuffer[1] = protocol_buff[17]>>8;
		cLaserRefBuffer[0] = protocol_buff[17]&0x00FF;
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
        
		cCounterOneBuffer[3] = protocol_buff[9]>>8;
		cCounterOneBuffer[2] = protocol_buff[9]&0x00FF;
		cCounterOneBuffer[1] = protocol_buff[10]>>8;
		cCounterOneBuffer[0] = protocol_buff[10]&0x00FF;
        
		cCounterTwoBuffer[3] = protocol_buff[11]>>8;
		cCounterTwoBuffer[2] = protocol_buff[11]&0x00FF;
		cCounterTwoBuffer[1] = protocol_buff[12]>>8;
		cCounterTwoBuffer[0] = protocol_buff[12]&0x00FF;
        
		cCounterThreeBuffer[3] = protocol_buff[13]>>8;
		cCounterThreeBuffer[2] = protocol_buff[13]&0x00FF;
		cCounterThreeBuffer[1] = protocol_buff[14]>>8;
		cCounterThreeBuffer[0] = protocol_buff[14]&0x00FF;
        
		cCounterFiveBuffer[3] = protocol_buff[15]>>8;
		cCounterFiveBuffer[2] = protocol_buff[15]&0x00FF;
		cCounterFiveBuffer[1] = protocol_buff[16]>>8;
		cCounterFiveBuffer[0] = protocol_buff[16]&0x00FF;
        
		cLaserRefBuffer[3] = protocol_buff[17]>>8;
		cLaserRefBuffer[2] = protocol_buff[17]&0x00FF;
		cLaserRefBuffer[1] = protocol_buff[18]>>8;
		cLaserRefBuffer[0] = protocol_buff[18]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁

	// printf("DELTAF_DataOutput::cO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	// 	cO2ppbBuffer[0], cO2ppbBuffer[1], cO2ppbBuffer[2], cO2ppbBuffer[3]);
	// printf("DELTAF_DataOutput::cHO2ppbBuffer = [%02X, %02X, %02X, %02X]\r\n", 
	// 	cHO2ppbBuffer[0], cHO2ppbBuffer[1], cHO2ppbBuffer[2], cHO2ppbBuffer[3]);
	
    sprintf(strOutput, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%.1f",
          *floatCounterZeroPointOneBuffer,     // 30002	Real	Counter 0.1 / Probe 1
          *floatCounterZeroPointTwoBuffer,     // 30004	Real	Counter 0.2 / Probe 1
          *floatCounterZeroPointThreeBuffer,   // 30006	Real	Counter 0.3 / Probe 1
          *floatCounterZeroPointFiveBuffer,    // 30008	Real	Counter 0.5 / Probe 1
          *floatCounterOneBuffer,              // 30010	Real	Counter 1 / Probe 1
          *floatCounterTwoBuffer,              // 30012	Real	Counter 2 / Probe 1
          *floatCounterThreeBuffer,            // 30014	Real	Counter 3 / Probe 1
          *floatCounterFiveBuffer,             // 30016	Real	Counter 5 / Probe 1
          *floatLaserRefBuffer,                // 30018	Real	Laser Ref. / Probe 1
          0.0);
	return 0;
}

uint16_t PMS_PDS_E_DataColumnsNumber()
{
	return 9;
}

