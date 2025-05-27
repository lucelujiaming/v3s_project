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
//    USART3_SetRcvMode(USART_RCV_DELAY,50,10);
    Instrument_USART_SetRcvMode(USART_RCV_DELAY,50,10);
//    USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
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

        start_pos= strstr(start_pos,crlf);    //DateTime 1
        start_pos= start_pos+2;
        start_pos= strstr(start_pos,crlf);    //DateTime 2
        start_pos= start_pos+2;
        start_pos= strstr(start_pos,crlf);    //Analyse
        start_pos= start_pos+2;

        while(1)
        {
            str_pos= strstr(start_pos,crlf);    //concentation

            if(str_pos!= NULL && str_pos< stop_pos)
            {
                //Name
                med_pos= start_pos;
                tab_pos= strstr(med_pos,tab);
                size= tab_pos - med_pos;
    			pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
                str_ptr= (uint8_t*)(&protocol_buff[1 + 8 * j]);
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
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
				
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
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
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
                //Alarm
                med_pos= tab_pos+1;
                tab_pos= strstr(med_pos,tab);
                size= tab_pos-med_pos;

                memset(data_temp,0,20);
                strncpy(data_temp,(const char*)(med_pos),size);
                data= atoi(data_temp);
				pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
                protocol_buff[8+8*j]= data;
    			pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
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

/*
    ORTHODYNE：DID550,，ORTHOSMART
    Register   Type   Description
    30001      Int    Communication Fault
    30002      Int    Name / GAS1
    30006      Real    Concent / GAS1
    30008      Int    Uint    ppb
    30009      Int    Alarm
    30010      Int    Name / GAS2
    30014      Real    Concent / GAS2
    30016      Int    Uint    ppb
    30017      Int    Alarm
    30018      Int    Name / GAS3
    30022      Real    Concent / GAS3
    30024      Int    Uint    ppb
    30025      Int    Alarm
    30026      Int    Name / GAS4
    30030      Real    Concent / GAS4
    30032      Int    Uint    ppb
    30033      Int    Alarm
    30034      Int    Name / GAS5
    30038      Real    Concent / GAS5
    30040      Int    Uint    ppb
    30041      Int    Alarm
    30042      Int    Name / GAS6
    30046      Real    Concent / GAS6
    30048      Int    Uint
    30049      Int    Alarm
 */
uint16_t ORTHODYNE_DataOutput(char* strOutput)
{
    char cConcentBuffer[4] = {0};
	float * floatConcentBufferPtr   = (float *)(cConcentBuffer);

    float  floatConcent[6];

    pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
        // Concent / GAS1
		cConcentBuffer[3] = protocol_buff[6]>>8;
		cConcentBuffer[2] = protocol_buff[6]&0x00FF;
		cConcentBuffer[1] = protocol_buff[5]>>8;
		cConcentBuffer[0] = protocol_buff[5]&0x00FF;
        floatConcent[0] = *floatConcentBufferPtr;
        // Concent / GAS2
		cConcentBuffer[3] = protocol_buff[14]>>8;
		cConcentBuffer[2] = protocol_buff[14]&0x00FF;
		cConcentBuffer[1] = protocol_buff[13]>>8;
		cConcentBuffer[0] = protocol_buff[13]&0x00FF;
        floatConcent[1] = *floatConcentBufferPtr;
        // Concent / GAS3
		cConcentBuffer[3] = protocol_buff[22]>>8;
		cConcentBuffer[2] = protocol_buff[22]&0x00FF;
		cConcentBuffer[1] = protocol_buff[21]>>8;
		cConcentBuffer[0] = protocol_buff[21]&0x00FF;
        floatConcent[2] = *floatConcentBufferPtr;
        // Concent / GAS4
		cConcentBuffer[3] = protocol_buff[30]>>8;
		cConcentBuffer[2] = protocol_buff[30]&0x00FF;
		cConcentBuffer[1] = protocol_buff[29]>>8;
		cConcentBuffer[0] = protocol_buff[29]&0x00FF;
        floatConcent[3] = *floatConcentBufferPtr;
        // Concent / GAS5
		cConcentBuffer[3] = protocol_buff[38]>>8;
		cConcentBuffer[2] = protocol_buff[38]&0x00FF;
		cConcentBuffer[1] = protocol_buff[37]>>8;
		cConcentBuffer[0] = protocol_buff[37]&0x00FF;
        floatConcent[4] = *floatConcentBufferPtr;
        // Concent / GAS6
		cConcentBuffer[3] = protocol_buff[46]>>8;
		cConcentBuffer[2] = protocol_buff[46]&0x00FF;
		cConcentBuffer[1] = protocol_buff[45]>>8;
		cConcentBuffer[0] = protocol_buff[45]&0x00FF;
        floatConcent[5] = *floatConcentBufferPtr;
    }
    else
	{
        // Concent / GAS1
		cConcentBuffer[3] = protocol_buff[5]>>8;
		cConcentBuffer[2] = protocol_buff[5]&0x00FF;
		cConcentBuffer[1] = protocol_buff[6]>>8;
		cConcentBuffer[0] = protocol_buff[6]&0x00FF;
        floatConcent[0] = *floatConcentBufferPtr;
        // Concent / GAS2
		cConcentBuffer[3] = protocol_buff[13]>>8;
		cConcentBuffer[2] = protocol_buff[13]&0x00FF;
		cConcentBuffer[1] = protocol_buff[14]>>8;
		cConcentBuffer[0] = protocol_buff[14]&0x00FF;
        floatConcent[1] = *floatConcentBufferPtr;
        // Concent / GAS3
		cConcentBuffer[3] = protocol_buff[21]>>8;
		cConcentBuffer[2] = protocol_buff[21]&0x00FF;
		cConcentBuffer[1] = protocol_buff[22]>>8;
		cConcentBuffer[0] = protocol_buff[22]&0x00FF;
        floatConcent[2] = *floatConcentBufferPtr;
        // Concent / GAS4
		cConcentBuffer[3] = protocol_buff[29]>>8;
		cConcentBuffer[2] = protocol_buff[29]&0x00FF;
		cConcentBuffer[1] = protocol_buff[30]>>8;
		cConcentBuffer[0] = protocol_buff[30]&0x00FF;
        floatConcent[3] = *floatConcentBufferPtr;
        // Concent / GAS5
		cConcentBuffer[3] = protocol_buff[37]>>8;
		cConcentBuffer[2] = protocol_buff[37]&0x00FF;
		cConcentBuffer[1] = protocol_buff[38]>>8;
		cConcentBuffer[0] = protocol_buff[38]&0x00FF;
        floatConcent[4] = *floatConcentBufferPtr;
        // Concent / GAS6
		cConcentBuffer[3] = protocol_buff[45]>>8;
		cConcentBuffer[2] = protocol_buff[45]&0x00FF;
		cConcentBuffer[1] = protocol_buff[46]>>8;
		cConcentBuffer[0] = protocol_buff[46]&0x00FF;
        floatConcent[5] = *floatConcentBufferPtr;
    }
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%f,%f,%f,%f,%.1f,%.1f,%.1f,%.1f",
          floatConcent[0],    // Real    Concent / GAS1
          floatConcent[1],    // Real    Concent / GAS2
          floatConcent[2],    // Real    Concent / GAS3
          floatConcent[3],    // Real    Concent / GAS4
          floatConcent[4],    // Real    Concent / GAS5
          floatConcent[5],    // Real    Concent / GAS6
          0.0, 0.0, 0.0, 0.0);
    return 0;
}

uint16_t ORTHODYNE_DataColumnsNumber()
{
	return 6;
}

