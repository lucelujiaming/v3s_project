//nanochrome.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "jag.h"

#define NANO_CMD_MAX        2

static uint8_t cmd_cnt;

char NANO_Cmd[NANO_CMD_MAX][5]= 
{
    {'B','B'},
    {'C','C'},
};

uint8_t nanochrome_check_sum(uint16_t len)
{
    uint8_t check_sum= 0;
    uint16_t i;
    
    for(i= 0; i< (len-3);i++)
    {
        check_sum+= serial_buff[1+i];
    }
    
    if(check_sum== serial_buff[len - 2])
    {
        return 1;
    }
    
    return 0;
}

void SERVOMEX_NANO_Init(int fd, uint16_t addr)
{
//    USART3_SetRcvMode(USART_RCV_CHAR,'>', '<');
    Instrument_USART_SetRcvMode(USART_RCV_CHAR,'>', '<');
//    USART3_Configuration(USART_BAUD(38400), PARITY_NONE, STB_1);
    set_speed(fd, 38400);
    if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
        printf("Set Parity Error\n");
        exit (0);
    }
}

uint16_t SERVOMEX_NANO_Request(void)
{
    uint8_t len;
    
    if(++cmd_cnt>= NANO_CMD_MAX)
    {
        cmd_cnt= 0;
    }
    
    len= strlen(NANO_Cmd[cmd_cnt]);
    memmove(serial_buff,(const char*)NANO_Cmd[cmd_cnt],len);
    
    return len;
}

//Concentration Unit:PPB
uint8_t SERVOMEX_NANO_Analysis(uint16_t len)
{
    uint8_t i,groups;
    uint16_t data;
    uint8_t err;
    float f_value;
    uint32_t u32_value;
    uint8_t *buff;
    
    err= 1;
    buff= serial_buff;
    if(nanochrome_check_sum(len))
    {
        err= 0;
        buff++;
        
        switch(cmd_cnt)
        {
        case 0:
            if(*buff != 'B')
            {
                break;
            }
            buff++;
            
            if(*buff == '1')
            {
                buff++;
                data= 0;
                
                for(i=0; i< 9;i++)
                {
                    if(*buff == '1')
                    {
                        data|= (uint16_t)0x01 << i;
                    }
                    buff++;
                }
                
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                protocol_buff[1]= data;
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
            }
            break;
        case 1:
            if(*buff != 'C')
            {
                break;
            }
            buff++;
            
            if(*buff == '1')
            {
                buff++;
                if(((len - 5)%15)== 0)
                {
                    groups= (len - 5)/15;
                    
                    // Get AI Value
                    for(i= 0;i<groups;i++)
                    {
                        memset(data_temp,0,20);
                        strncpy(data_temp,(const char*)buff,10);
                        
                        f_value= atof(data_temp);
                        u32_value= real_to_u32(f_value);
                        
						pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                        if(little_endian)
                        {
                            protocol_buff[2 + 4 * i]= _low_word_int32(u32_value);
                            protocol_buff[3 + 4 * i]= _high_word_int32(u32_value);
                        }
                        else
                        {
                            protocol_buff[2 + 4 * i]= _high_word_int32(u32_value);
                            protocol_buff[3 + 4 * i]= _low_word_int32(u32_value);
                        }
    					pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
                        buff+= 10;
                        
                        //
                        if(*buff == ';')
                        {
                            buff++;
                            
							pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                            protocol_buff[4 + 4*i]= *buff - '0';
    						pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
                            buff++;
                        }
                        else
                        {
                            break;
                        }
                        
                        if(*buff == ';')
                        {
                            buff++;
                            
							pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                            protocol_buff[5 + 4*i]= *buff- '0';
    						pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
                            buff++;
                        }
                        else
                        {
                            break;
                        }
                        
                        if(*buff != ';')
                        {
                            break;
                        }
                        buff++;
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    return err;
}

/*
    Register    Type    Description
    30001       Int     Communication Fault
    30002       Int     Status
    30003       Real    Concentration_0
    30005       Int     Unit_0
    30006       Int     Peak_Valid_0
    30007       Real    Concentration_1
    30009       Int     Unit_1
    30010       Int     Peak_Valid_1
    30011       Real    Concentration_2
    30013       Int     Unit_2
    30014       Int     Peak_Valid_2
    30015       Real    Concentration_3
    30017       Int     Unit_3
    30018       Int     Peak_Valid_3
    30019       Real    Concentration_4
    30021       Int     Unit_4
    30022       Int     Peak_Valid_4
    30023       Real    Concentration_5
    30025       Int     Unit_5
    30026       Int     Peak_Valid_5
    30027       Real    Concentration_6
    30029       Int     Unit_6
    30030       Int     Peak_Valid_6
    30031       Real    Concentration_7
    30033       Int     Unit_7
    30034       Int     Peak_Valid_7
    30035       Real    Concentration_8
    30037       Int     Unit_8
    30038       Int     Peak_Valid_8
    30039       Real    Concentration_9
    30041       Int     Unit_9
    30042       Int     Peak_Valid_9
 */
uint16_t SERVOMEX_NANO_DataOutput(char* strOutput)
{
    char cConcentrationBuffer[4] = {0};
	float * floatConcentrationrationBufferPtr   = (float *)(cConcentrationBuffer);

    float  floatConcentration[10];

    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
        // Concentration_0
		cConcentrationBuffer[3] = protocol_buff[3]>>8;
		cConcentrationBuffer[2] = protocol_buff[3]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[2]>>8;
		cConcentrationBuffer[0] = protocol_buff[2]&0x00FF;
        floatConcentration[0] = *floatConcentrationrationBufferPtr;
        // Concentration_1
		cConcentrationBuffer[3] = protocol_buff[7]>>8;
		cConcentrationBuffer[2] = protocol_buff[7]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[6]>>8;
		cConcentrationBuffer[0] = protocol_buff[6]&0x00FF;
        floatConcentration[1] = *floatConcentrationrationBufferPtr;
        // Concentration_2
		cConcentrationBuffer[3] = protocol_buff[11]>>8;
		cConcentrationBuffer[2] = protocol_buff[11]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[10]>>8;
		cConcentrationBuffer[0] = protocol_buff[10]&0x00FF;
        floatConcentration[2] = *floatConcentrationrationBufferPtr;
        // Concentration_3
		cConcentrationBuffer[3] = protocol_buff[15]>>8;
		cConcentrationBuffer[2] = protocol_buff[15]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[14]>>8;
		cConcentrationBuffer[0] = protocol_buff[14]&0x00FF;
        floatConcentration[3] = *floatConcentrationrationBufferPtr;
        // Concentration_4
		cConcentrationBuffer[3] = protocol_buff[19]>>8;
		cConcentrationBuffer[2] = protocol_buff[19]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[18]>>8;
		cConcentrationBuffer[0] = protocol_buff[18]&0x00FF;
        floatConcentration[4] = *floatConcentrationrationBufferPtr;
        // Concentration_5
		cConcentrationBuffer[3] = protocol_buff[23]>>8;
		cConcentrationBuffer[2] = protocol_buff[23]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[22]>>8;
		cConcentrationBuffer[0] = protocol_buff[22]&0x00FF;
        floatConcentration[5] = *floatConcentrationrationBufferPtr;
        // Concentration_6
		cConcentrationBuffer[3] = protocol_buff[27]>>8;
		cConcentrationBuffer[2] = protocol_buff[27]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[26]>>8;
		cConcentrationBuffer[0] = protocol_buff[26]&0x00FF;
        floatConcentration[6] = *floatConcentrationrationBufferPtr;
        // Concentration_7
		cConcentrationBuffer[3] = protocol_buff[31]>>8;
		cConcentrationBuffer[2] = protocol_buff[31]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[30]>>8;
		cConcentrationBuffer[0] = protocol_buff[30]&0x00FF;
        floatConcentration[7] = *floatConcentrationrationBufferPtr;
        // Concentration_8
		cConcentrationBuffer[3] = protocol_buff[35]>>8;
		cConcentrationBuffer[2] = protocol_buff[35]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[34]>>8;
		cConcentrationBuffer[0] = protocol_buff[34]&0x00FF;
        floatConcentration[8] = *floatConcentrationrationBufferPtr;
        // Concentration_9
		cConcentrationBuffer[3] = protocol_buff[39]>>8;
		cConcentrationBuffer[2] = protocol_buff[39]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[38]>>8;
		cConcentrationBuffer[0] = protocol_buff[38]&0x00FF;
        floatConcentration[9] = *floatConcentrationrationBufferPtr;
    }
    else
	{
        // Concentration_0
		cConcentrationBuffer[3] = protocol_buff[2]>>8;
		cConcentrationBuffer[2] = protocol_buff[2]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[3]>>8;
		cConcentrationBuffer[0] = protocol_buff[3]&0x00FF;
        floatConcentration[0] = *floatConcentrationrationBufferPtr;
        // Concentration_1
		cConcentrationBuffer[3] = protocol_buff[6]>>8;
		cConcentrationBuffer[2] = protocol_buff[6]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[7]>>8;
		cConcentrationBuffer[0] = protocol_buff[7]&0x00FF;
        floatConcentration[1] = *floatConcentrationrationBufferPtr;
        // Concentration_2
		cConcentrationBuffer[3] = protocol_buff[10]>>8;
		cConcentrationBuffer[2] = protocol_buff[10]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[11]>>8;
		cConcentrationBuffer[0] = protocol_buff[11]&0x00FF;
        floatConcentration[2] = *floatConcentrationrationBufferPtr;
        // Concentration_3
		cConcentrationBuffer[3] = protocol_buff[14]>>8;
		cConcentrationBuffer[2] = protocol_buff[14]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[15]>>8;
		cConcentrationBuffer[0] = protocol_buff[15]&0x00FF;
        floatConcentration[3] = *floatConcentrationrationBufferPtr;
        // Concentration_4
		cConcentrationBuffer[3] = protocol_buff[18]>>8;
		cConcentrationBuffer[2] = protocol_buff[18]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[19]>>8;
		cConcentrationBuffer[0] = protocol_buff[19]&0x00FF;
        floatConcentration[4] = *floatConcentrationrationBufferPtr;
        // Concentration_5
		cConcentrationBuffer[3] = protocol_buff[22]>>8;
		cConcentrationBuffer[2] = protocol_buff[22]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[23]>>8;
		cConcentrationBuffer[0] = protocol_buff[23]&0x00FF;
        floatConcentration[5] = *floatConcentrationrationBufferPtr;
        // Concentration_6
		cConcentrationBuffer[3] = protocol_buff[26]>>8;
		cConcentrationBuffer[2] = protocol_buff[26]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[27]>>8;
		cConcentrationBuffer[0] = protocol_buff[27]&0x00FF;
        floatConcentration[6] = *floatConcentrationrationBufferPtr;
        // Concentration_7
		cConcentrationBuffer[3] = protocol_buff[30]>>8;
		cConcentrationBuffer[2] = protocol_buff[30]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[31]>>8;
		cConcentrationBuffer[0] = protocol_buff[31]&0x00FF;
        floatConcentration[7] = *floatConcentrationrationBufferPtr;
        // Concentration_8
		cConcentrationBuffer[3] = protocol_buff[34]>>8;
		cConcentrationBuffer[2] = protocol_buff[34]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[35]>>8;
		cConcentrationBuffer[0] = protocol_buff[35]&0x00FF;
        floatConcentration[8] = *floatConcentrationrationBufferPtr;
        // Concentration_9
		cConcentrationBuffer[3] = protocol_buff[38]>>8;
		cConcentrationBuffer[2] = protocol_buff[38]&0x00FF;
		cConcentrationBuffer[1] = protocol_buff[39]>>8;
		cConcentrationBuffer[0] = protocol_buff[39]&0x00FF;
        floatConcentration[9] = *floatConcentrationrationBufferPtr;
    }
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
          floatConcentration[0],    // Real    Concentration_0
          floatConcentration[1],    // Real    Concentration_1
          floatConcentration[2],    // Real    Concentration_2
          floatConcentration[3],    // Real    Concentration_3
          floatConcentration[4],    // Real    Concentration_4
          floatConcentration[5],    // Real    Concentration_5
          floatConcentration[6],    // Real    Concentration_6
          floatConcentration[7],    // Real    Concentration_7
          floatConcentration[8],    // Real    Concentration_8
          floatConcentration[9]     // Real    Concentration_9
          );
    return 0;
}

uint16_t SERVOMEX_NANO_DataColumnsNumber()
{
	return 10;
}

