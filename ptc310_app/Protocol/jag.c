//jag.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "jag.h"

#define JAG_CMD_MAX		1

static uint8_t cmd_cnt;

/*
char JAG_Cmd[JAG_CMD_MAX][10]= 
{
	{'R','E','A','D',0x0D,0X0A},
//	{'M','F','C',0x0D,0X0A},
//	{'C','_','T','E','M','P',0x0D,0X0A},
//	{'S','_','W','A','T','E','R',0x0D,0X0A}
};
 */
char * JAG_Cmd[4] = {
	"READ\x0D\x0A",
	"MFC\x0D\x0A",
	"C_TEMP\x0D\x0A",
	"S_WATER\x0D\x0A"
};

void JAG_Init(int fd, uint16_t addr)
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

uint16_t JAG_Request(void)
{
	uint8_t len;
	
	if(++cmd_cnt>= JAG_CMD_MAX)
	{
		cmd_cnt= 0;
	}
	
	len= strlen(JAG_Cmd[cmd_cnt]);
	memmove(serial_buff,(const char*)JAG_Cmd[cmd_cnt],len);
	
	return len;
}

//Concentration Unit:PPB
uint8_t JAG_Analysis(uint16_t len)
{
	uint8_t i = 0,j = 0,str_size = 0;
	uint16_t data = 0;
	uint8_t err = 0;
	float f_value = 0.0;
	uint32_t u32_value = 0;
	
	err= 1;
	
	if(serial_buff[len - 2]!= 0x0D
		|| serial_buff[len - 1]!= 0x0A)
	{
		return err;
	}
	
	switch(cmd_cnt)
	{
	case 0:	//Concentration & Status
		str_size= 0;
		i= 0;
		j=0;
		
		if(strncmp((const char*)serial_buff,(const char*)JAG_Cmd[0],4)== 0)
		{
			if(serial_buff[4]== ':')
			{
				while(i<len-5)
				{
					if(*(serial_buff+5+i)=='/')
					{
						if(++j>= 4)
						{
							i++;
							break;
						}
					}
					i++;
				}
				
				if(j==4)
				{
					for(j=0;j<len-5-i;j++)
					{
						if(*(serial_buff+5+i+j)== '/')
						{
							break;
						}
						str_size++;
					}
				}
			
				if(str_size)
				{
					//Measure Value
					memset(data_temp,0,20);
					strncpy((char*)data_temp,(const char*)(serial_buff+5+i),str_size-3);
					f_value= atof(data_temp);
					
					//Unit Change
					if(strncmp((const char*)(serial_buff+5+i+str_size-3),"ppt",3)== 0)
					{
						f_value/= 1000;
					}
					else
					{
						if(strncmp((const char*)(serial_buff+5+i+str_size-3),"ppm",3)== 0)
						{
							f_value*= 1000;
						}
					}
					
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
					
					//Status
//					if(*(serial_buff+ 5+ i+ str_size+ 2)== 'S')	//ST1 & ST2 & STOP
//					{
//						if(*(serial_buff+5+i+str_size+4)== '1')
//						{
//							protocol_buff[1]= 2;
//						}
//						else
//						{
//							if(*(serial_buff+5+i+str_size+4)== '2')
//							{
//								protocol_buff[1]= 3;
//							}
//							else
//							{
//								protocol_buff[1]= 0;
//							}
//						}
//					}
//					else
//					{
//						if(*(serial_buff+5+i+str_size+2)== 'M')		//MEA-
//						{
//							protocol_buff[1]= 1;
//						}
//					}
				}
				err= 0;
			}
		}
		break;
	case 1:		//MFC1 & MF2
		if(strncmp((const char*)serial_buff,(const char*)JAG_Cmd[1],3)== 0)
		{
			if(serial_buff[3]== ':')
			{
				str_size= 0;
				for(j=0;j<len-4;j++)
				{
					if(*(serial_buff+4+j)== '/')
					{
						break;
					}
					str_size++;
				}
				
				if(str_size)
				{
					memset(data_temp,0,20);
					strncpy((char*)data_temp,(const char*)(serial_buff+4),str_size);
					data= atoi(data_temp);
					
					pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
					protocol_buff[5]= data;
    				pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
					i= str_size;
				}
				
				
				str_size= 0;
				for(j=0;j<len-4-i;j++)
				{
					if(*(serial_buff+4+i+j+1)== 0x0D)
					{
						break;
					}
					str_size++;
				}
				
				if(str_size)
				{
					memset(data_temp,0,20);
					strncpy((char*)data_temp,(const char*)(serial_buff+4+i+1),str_size);
					data= atoi(data_temp);
					
					pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
					protocol_buff[6]= data;
    				pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
				}
				err= 0;
			}
		}
		break;
	case 2:	//C_TEMP
		if(strncmp((const char*)serial_buff,(const char*)JAG_Cmd[2],6)== 0)
		{
			if(serial_buff[6]== ':')
			{
				str_size= 0;
				for(j=0;j<len-7;j++)
				{
					if(*(serial_buff+7+j)== 0X0d)
					{
						break;
					}
					str_size++;
				}
				
				if(str_size)
				{
					memset(data_temp,0,20);
					strncpy((char*)data_temp,(const char*)(serial_buff+7),str_size);
					
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
				err= 0;
			}
		}
		break;
	case 3:	//S_WATER
		str_size= 0;
		i= 0;
		j=0;
		
		if(strncmp((const char*)serial_buff,(const char*)JAG_Cmd[3],7)== 0)
		{
			if(serial_buff[7]== ':')
			{
				if(serial_buff[8]== 'L')		//LOW
				{	
					pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
					protocol_buff[2]= 2;
    				pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
				}
				else
				{
					pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
					protocol_buff[2]= 1;
    				pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
				}
				err= 0;
			}
		}
		break;
	default:
		break;
	}

	return err;
}

/*
    JAG：MX1A，MXNY
    寄存器	    类型	功能描述	    数值
    30001	    Int	    Communication   Fault	0：Normal；1：Fault
    30002	    Int	    STATUS	        0：STOP；1：MEA-；2：ST1；3：ST2
    30003	    Int	    S_WATER	        1：OK；2：Low
    30004	    Real	Measured Value	Unit: ppb
    30006	    Int	    MFC1	        0-244CCM
    30007	    Int	    MFC2	        0-244CCM
    30008	    Real	C_TEMP	
 */
uint16_t JAG_DataOutput(char* strOutput)
{
	char cMeasuredValueBuffer[4] = {0};
	char cC_TEMPBuffer[4] = {0};
	float * floatMeasuredValue  = (float *)cMeasuredValueBuffer;
	float * floatC_TEMP = (float *)cC_TEMPBuffer;
    
	pthread_rwlock_rdlock(&ireg_rwlock); // 获取IReg的读锁
	if(little_endian)
	{
		cMeasuredValueBuffer[3] = protocol_buff[4]>>8;
		cMeasuredValueBuffer[2] = protocol_buff[4]&0x00FF;
		cMeasuredValueBuffer[1] = protocol_buff[3]>>8;
		cMeasuredValueBuffer[0] = protocol_buff[3]&0x00FF;
		
		cC_TEMPBuffer[3] = protocol_buff[8]>>8;
		cC_TEMPBuffer[2] = protocol_buff[8]&0x00FF;
		cC_TEMPBuffer[1] = protocol_buff[7]>>8;
		cC_TEMPBuffer[0] = protocol_buff[7]&0x00FF;
		//		printf("DELTAF_DataOutput::little_endian\r\n");
	}
	else
	{
		cMeasuredValueBuffer[3] = protocol_buff[3]>>8;
		cMeasuredValueBuffer[2] = protocol_buff[3]&0x00FF;
		cMeasuredValueBuffer[1] = protocol_buff[4]>>8;
		cMeasuredValueBuffer[0] = protocol_buff[4]&0x00FF;
		                                       
		cC_TEMPBuffer[3] = protocol_buff[7]>>8;
		cC_TEMPBuffer[2] = protocol_buff[7]&0x00FF;
		cC_TEMPBuffer[1] = protocol_buff[8]>>8;
		cC_TEMPBuffer[0] = protocol_buff[8]&0x00FF;
		//		printf("DELTAF_DataOutput::big_endian\r\n");
	}
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的读锁
    
    sprintf(strOutput, "%f,%f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",      
            *floatMeasuredValue, // 30004	    Real	Measured Value	
            *floatC_TEMP,        // 30006       Real    Flow Setpoint
            0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0);
    
	return 0;
}

uint16_t JAG_DataColumnsNumber()
{
	return 2;
}

