//ametek.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "jag.h"

#define AMETEK_2850_CMD_MAX        4

static uint8_t cmd_cnt;

char AMETEK_2850_Cmd[AMETEK_2850_CMD_MAX][7]=
{
    {0xC9,'R','0','1',0x0D,0x0A},
    {0xC9,'R','0','6',0x0D,0x0A},
    {0xC9,'R','1','0',0x0D,0x0A},
    {0xC9,'R','1','1',0x0D,0x0A}
};

void AMETEK_Init(int fd, uint16_t addr)
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

uint16_t AMETEK_2850_Request(void)
{
    uint8_t len;

    if(++cmd_cnt>= AMETEK_2850_CMD_MAX)
    {
        cmd_cnt= 0;
    }

    len= strlen(AMETEK_2850_Cmd[cmd_cnt]);
    memmove(serial_buff,(const char*)AMETEK_2850_Cmd[cmd_cnt], len);

    return len;
}

uint8_t AMETEK_2850_Analysis(uint16_t len)
{
    uint16_t data_1, data_2;
    uint8_t err;
    float f_value;
    uint32_t u32_value;

    err= 1;

    if(serial_buff[0]== 0x20 && serial_buff[len - 2]== 0x0D && serial_buff[len - 1]== 0x11)
    {
        err= 0;

        memset(data_temp,0,20);
        strncpy(data_temp,(char*)&serial_buff[1],len-3);
        f_value= atof(data_temp);
        u32_value= real_to_u32(f_value);

        if(little_endian)
        {
            data_1= _low_word_int32(u32_value);
            data_2= _high_word_int32(u32_value);
        }
        else
        {
            data_1= _high_word_int32(u32_value);
            data_2= _low_word_int32(u32_value);
        }
		
		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
        switch(cmd_cnt)
        {
        case 0:
            protocol_buff[1]= data_1;
            protocol_buff[2]= data_2;
            break;
        case 1:
            protocol_buff[3]= data_1;
            protocol_buff[4]= data_2;
            break;
        case 2:
            protocol_buff[5]= data_1;
            protocol_buff[6]= data_2;
            break;
        case 3:
            protocol_buff[7]= data_1;
            protocol_buff[8]= data_2;
            break;
        default:
            break;
        }
    	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
    }

    return err;
}

/*
    AMETEK：2850
    Register    Type    Description
    30001       Int     Communication Fault
    30002       Int     Year
    30003       Byte    Month
    30003       Byte    Date
    30004       Byte    Hour
    30004       Byte    Minute
    30005       Byte    Second
    30005       Byte    Run/Maintenance
    30006       Byte    EDL/Single
    30006       Byte    Alarm
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
uint16_t AMETEK_2850_DataOutput(char* strOutput)
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

uint16_t AMETEK_2850_DataColumnsNumber()
{
	return 6;
}

/////////////////////////////////////////////
uint16_t AMETEK_5000_Request(void)
{
    uint8_t len;

    len= 10;
    strncpy((char*)serial_buff, ">F0F5B33\r\n", len);

    return len;
}

uint8_t AMETEK_5000_Analysis(uint16_t len)
{
    uint8_t err;
    float f_value;
    uint32_t u32_value;

    err= 1;

    if(serial_buff[0]== 'A' && serial_buff[len - 1]== 0x0D)
    {
        err= 0;
        memset(data_temp,0,20);
        strncpy(data_temp,(const char*)(serial_buff + 1),len - 4);

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

    return err;
}

/*
    AMETEK：5830，5920，3050
    Register    Type    Description
    30001       Int     Communication Fault
    30002       Int     Year
    30003       Byte    Month
    30003       Byte    Date
    30004       Byte    Hour
    30004       Byte    Minute
    30005       Byte    Second
    30005       Byte    Run/Maintenance
    30006       Byte    EDL/Single
    30006       Byte    Alarm
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
uint16_t AMETEK_5000_DataOutput(char* strOutput)
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

uint16_t AMETEK_5000_DataColumnsNumber()
{
	return 6;
}

