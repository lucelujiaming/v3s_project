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
//    USART3_SetRcvMode(USART_RCV_DELAY,50,10);
    Instrument_USART_SetRcvMode(USART_RCV_DELAY,50,10);
//    USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
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
        	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
            protocol_buff[1]= 1;
        	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
        }
        else
        {
            if((*start_pos)== 'R')
            {
        		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                protocol_buff[1]= 0;
        		pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
        	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
            protocol_buff[2]= 1;
        	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
        }
        else
        {
            if((*start_pos)== 'E')
            {
        		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                protocol_buff[2]= 0;
        		pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
    	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
        protocol_buff[3]= 0;
        pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
						pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
						protocol_buff[3]|= 0x01 << (7 - j);
						pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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
   			pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
            str_ptr= (uint8_t*)(&protocol_buff[6 + 5*j]);
    		pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
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

            //Concern    ,30010
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
                
        		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
        		pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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

/*
    SAES：TA7000，TA5000
    Register    Type    Description    Value
    30001       Int     Communication Fault
    30002       Int     Year
    30003       Int     Run/Maintenance
    30004       Int     EDL/Single
    30005       Int     Alarm
    30006       Int
    30007       Byte    Name of Gas1
    30010       Real    Concentration of Gas1
    30012       Byte    Name of Gas2
    30015       Real    Concentration of Gas2
    30017       Byte    Name of Gas3
    30020       Real    Concentration of Gas3
    30022       Byte    Name of Gas4
    30025       Real    Concentration of Gas4
    30027       Byte    Name of Gas5
    30030       Real    Concentration of Gas5    
    30032       Byte    Name of Gas6
    30035       Real    Concentration of Gas6    
 */
uint16_t SAES_DataOutput(char* strOutput)
{
    char cConcentrationBuffer[4] = {0};
	float * floatConcentrationBufferPtr   = (float *)(cConcentrationBuffer);
    
    float  floatConcentration[6];
    
    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
        // Concentration / GAS1
		cConcentrationBuffer[3] = protocol_buff[10]>>8;
		cConcentrationBuffer[2] = protocol_buff[10]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[9]>>8;
		cConcentrationBuffer[0] = protocol_buff[9]&0x00FF;
        floatConcentration[0] = *floatConcentrationBufferPtr;
        // Concentration / GAS2
		cConcentrationBuffer[3] = protocol_buff[15]>>8;
		cConcentrationBuffer[2] = protocol_buff[15]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[14]>>8;
		cConcentrationBuffer[0] = protocol_buff[14]&0x00FF;
        floatConcentration[1] = *floatConcentrationBufferPtr;
        // Concentration / GAS3
		cConcentrationBuffer[3] = protocol_buff[20]>>8;
		cConcentrationBuffer[2] = protocol_buff[20]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[19]>>8;
		cConcentrationBuffer[0] = protocol_buff[19]&0x00FF;
        floatConcentration[2] = *floatConcentrationBufferPtr;
        // Concentration / GAS4
		cConcentrationBuffer[3] = protocol_buff[25]>>8;
		cConcentrationBuffer[2] = protocol_buff[25]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[24]>>8;
		cConcentrationBuffer[0] = protocol_buff[24]&0x00FF;
        floatConcentration[3] = *floatConcentrationBufferPtr;
        // Concentration / GAS5
		cConcentrationBuffer[3] = protocol_buff[30]>>8;
		cConcentrationBuffer[2] = protocol_buff[30]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[29]>>8;
		cConcentrationBuffer[0] = protocol_buff[29]&0x00FF;
        floatConcentration[4] = *floatConcentrationBufferPtr;
        // Concentration / GAS6
		cConcentrationBuffer[3] = protocol_buff[35]>>8;
		cConcentrationBuffer[2] = protocol_buff[35]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[34]>>8;
		cConcentrationBuffer[0] = protocol_buff[34]&0x00FF;
        floatConcentration[5] = *floatConcentrationBufferPtr;
    }
    else
	{
        // Concentration / GAS1
		cConcentrationBuffer[3] = protocol_buff[9]>>8;
		cConcentrationBuffer[2] = protocol_buff[9]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[10]>>8;
		cConcentrationBuffer[0] = protocol_buff[10]&0x00FF;
        floatConcentration[0] = *floatConcentrationBufferPtr;
        // Concentration / GAS2
		cConcentrationBuffer[3] = protocol_buff[14]>>8;
		cConcentrationBuffer[2] = protocol_buff[14]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[15]>>8;
		cConcentrationBuffer[0] = protocol_buff[15]&0x00FF;
        floatConcentration[1] = *floatConcentrationBufferPtr;
        // Concentration / GAS3
		cConcentrationBuffer[3] = protocol_buff[19]>>8;
		cConcentrationBuffer[2] = protocol_buff[19]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[20]>>8;
		cConcentrationBuffer[0] = protocol_buff[20]&0x00FF;
        floatConcentration[2] = *floatConcentrationBufferPtr;
        // Concentration / GAS4
		cConcentrationBuffer[3] = protocol_buff[24]>>8;
		cConcentrationBuffer[2] = protocol_buff[24]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[25]>>8;
		cConcentrationBuffer[0] = protocol_buff[25]&0x00FF;
        floatConcentration[3] = *floatConcentrationBufferPtr;
        // Concentration / GAS5
		cConcentrationBuffer[3] = protocol_buff[29]>>8;
		cConcentrationBuffer[2] = protocol_buff[29]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[30]>>8;
		cConcentrationBuffer[0] = protocol_buff[30]&0x00FF;
        floatConcentration[4] = *floatConcentrationBufferPtr;
        // Concentration / GAS6
		cConcentrationBuffer[3] = protocol_buff[34]>>8;
		cConcentrationBuffer[2] = protocol_buff[34]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[35]>>8;
		cConcentrationBuffer[0] = protocol_buff[35]&0x00FF;
        floatConcentration[5] = *floatConcentrationBufferPtr;
    }
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%f,%f,%f,%f,%.1f,%.1f,%.1f,%.1f",
          floatConcentration[0],    // Real    Concentration / GAS1	
          floatConcentration[1],    // Real    Concentration / GAS2	
          floatConcentration[2],    // Real    Concentration / GAS3	
          floatConcentration[3],    // Real    Concentration / GAS4	
          floatConcentration[4],    // Real    Concentration / GAS5	
          floatConcentration[5],    // Real    Concentration / GAS6	
          0.0, 0.0, 0.0, 0.0);
    return 0;
}

uint16_t SAES_DataColumnsNumber()
{
	return 6;
}

