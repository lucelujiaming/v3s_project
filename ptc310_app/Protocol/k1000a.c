//K1000.c
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "protocol.h"
#include "usart.h"
#include "usart_type.h"
#include "k1000a.h"

const uint16_t k1000a_crc_table[256] = 
{ /* CRC 余式表 */
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

uint16_t cal2_crc16(uint8_t *buff, uint8_t len) 
{
	uint16_t crc = 0;
	uint8_t da = 0;
	
	crc = 0;
	while (len-- != 0) 
	{
		if(*buff!= 0x09)
		{
			da = (uint8_t)(crc / 256);/* 以8位二进制数的形式暂存CRC的高8位 */
			crc <<= 8; 		/* 左移8位,相当于CRC的低8位乘以28 */
			crc ^= k1000a_crc_table[ da ^ *buff ]; /* 高8位和当前字节相加后再查表求CRC,再加上以前的CRC */
		}
		buff++;
	}
	return (crc);
} 
//////////////////////////////////////////////////
void SERVOMEX_K1000A_Init(int fd, uint16_t addr)
{
//	USART3_SetRcvMode(USART_RCV_DELAY,50,10);
    Convert_USART_SetRcvMode(USART_RCV_DELAY,50,10);
//	USART3_Configuration(USART_BAUD(9600), PARITY_NONE, STB_1);
    set_speed(fd, 9600);
	if (set_parity(fd, 8, 1, 'N') == SERIAL_FALSE)  {
		printf("Set Parity Error\n");
		exit (0);
	}
}

uint8_t SERVOMEX_K1000A_Analysis(uint16_t len)
{
	float f_value;
	uint32_t u32_value;
	uint16_t crc;
	char *ptr,*eptr;
	
	crc= cal2_crc16(serial_buff, len - 4);
	
	if(serial_buff[len-1]== 0x0A && (crc >> 8)== serial_buff[len- 4] && (crc & 0xff)==serial_buff[len- 3])
	{
		//Impurity
		ptr= (char*)&serial_buff[0];
		
		eptr= strchr(ptr,0x09);
		if(eptr== NULL)
		{
			goto ERROR_EXIT;
		}
		
		memset(data_temp,0,20);
		strncpy(data_temp,ptr,eptr - ptr);
		
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
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
		
		ptr= eptr + 1;
		//Air Flow
		eptr= strchr(ptr,0x09);
		if(eptr== NULL)
		{
			goto ERROR_EXIT;
		}
		
		memset(data_temp,0,20);
		strncpy(data_temp,ptr,eptr - ptr);
		
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
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
		ptr= eptr + 1;
		//Feul Flow
		eptr= strchr(ptr,0x09);
		if(eptr== NULL)
		{
			goto ERROR_EXIT;
		}
		
		memset(data_temp,0,20);
		strncpy(data_temp,ptr,eptr - ptr);
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
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
		
		ptr= eptr + 1;
		//Sample Flow
		eptr= strchr(ptr,0x09);
		if(eptr== NULL)
		{
			goto ERROR_EXIT;
		}
		
		memset(data_temp,0,20);
		strncpy(data_temp,ptr,eptr - ptr);
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
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
		
		ptr= eptr + 1;
		//
		eptr= strchr(ptr,0x09);
		if(eptr== NULL)
		{
			goto ERROR_EXIT;
		}
		ptr= eptr + 1;
		//FID Temp
		eptr= strchr(ptr,0x09);
		if(eptr== NULL)
		{
			goto ERROR_EXIT;
		}
		
		memset(data_temp,0,20);
		strncpy(data_temp,ptr,eptr - ptr);
		f_value= atof(data_temp);
		u32_value= real_to_u32(f_value);
		
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
	}
	else
	{
		goto ERROR_EXIT;
	}
	
	return 0;
	
ERROR_EXIT:	
	return 1;
}
