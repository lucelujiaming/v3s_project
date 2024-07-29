// Version:1.0

#include "common.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t _abs_int32(int32_t data)
{
	return (data>0?data:-data);
}

uint16_t _abs_int16(int16_t data)
{
	return (data>0?data:-data);
}

float _abs_float(float data)
{
	return (data>0?data:-data);
}
/////////////////////////////////////////////////////////
uint16_t _get_int16_int8_big_endian(uint8_t* pdata)
{
	return ((((uint16_t)(*pdata)<<8)&0xFF00)|*(pdata+1));
}

uint16_t _get_int16_int8_little_endian(uint8_t* pdata)
{
	return ((((uint16_t)(*(pdata+1))<<8)&0xFF00)|*(pdata));
}
/////////////////////////////////////////////////////////
uint32_t _get_int32_int8_modbus(uint8_t* pdata)
{
	uint16_t high_word,low_word;
	
	low_word= _get_int16_int8_big_endian(pdata);
	high_word= _get_int16_int8_big_endian(pdata+2);
	
	return ((((uint32_t)(high_word)<<16)&0xFFFF0000) | low_word);
}

uint32_t _get_int32_int16_modbus(uint16_t* pdata)
{
	return ((((uint32_t)(*(pdata+1))<<16)&0xFFFF0000) | (*pdata));
}

void _set_int16_int32_modbus(uint16_t* pdata,uint32_t data)
{
	*pdata= data & 0x0000FFFF;
	*(pdata+1)= (data>>16) & 0x0000FFFF;
}
/////////////////////////////////////////////////////////
uint32_t _get_int32_int8_mbus(uint8_t* pdata)
{
	uint32_t high_word,low_word;
	
	low_word= (uint32_t)_get_int16_int8_little_endian(pdata);
	high_word= (uint32_t)_get_int16_int8_little_endian(pdata+2);
	
	return ((((high_word)<<16)&0xFFFF0000) | low_word);
}

void _set_int8_int32_mbus(uint8_t* pdata,uint32_t value)
{
	uint8_t i;

	for(i= 0;i<4;i++)
	{
		*(pdata+i)= value & 0x000000FF;
		value>>= 8;
	}
}

uint16_t _swap_int16(uint16_t data)
{
	return (((data<<8)&0xFF00)|((data>>8)&0x00FF));
}

uint8_t _swap_int8(uint8_t data)
{
	return (((data<<4)&0xF0)|((data>>4)&0x0F));
}
///////////////////////////////////////////////////////
uint8_t _high_byte_int16(uint16_t data)
{
	return (uint8_t)(data>>8);
}

uint8_t _low_byte_int16(uint16_t data)
{
	return (uint8_t)(data & 0x00FF);
}

uint16_t _high_word_int32(uint32_t data)
{
	return (uint16_t)((data>>16)&0x0000FFFF);
}

uint16_t _low_word_int32(uint32_t data)
{
	return (uint16_t)(data&0x0000FFFF);
}
///////////////////////////////////////////////////////////
uint16_t _BV16(uint8_t i)
{
	return ((uint16_t)1<<i);
}

uint32_t _BV32(uint8_t i)
{
	return ((uint32_t)1<<i);
}

void delay_ms(uint16_t time)
{    
   uint16_t i=0;  
   while(time--)
   {
      i=18000;  //Òü¹¤ÓÃµÄ18000
      while(i--) ;    
   }
}

float analog_ramp(float pv, float set, float max_delta)
{
	float ret_value;
	
	if(fabs(set - pv) > max_delta)
	{
		if(set > pv)
		{
			ret_value= pv + max_delta;
		}
		else
		{
			ret_value= pv - max_delta;
		}
	}
	else
	{
		ret_value= set;
	}
	
	return ret_value;
}

float hi_lo_limit(float pv, float hi, float lo)
{
	if(pv > hi)
	{
		pv= hi;
	}
	else
	{
		if(pv < lo)
		{
			pv= lo;
		}
	}
	
	return pv;
}

// 0.0001 sqm
float get_area_round(float diameter)	
{
	return (3.1416f * pow(diameter/2.0f, 2));
}

char* strnstr(char* src, char ch, uint8_t len)
{
	char* ptr;
	char temp[256];
	
	if(!len)
		return NULL;
	
	memset(temp, 0,11);
	memcpy(temp, src, len);
	
	ptr= strchr((char*)temp, ch);
	
	if(ptr && ptr - temp < len)
	{
		return (char*)(src + (ptr - temp));
	}
	
	return NULL;
}

char* str_int(char* src, char ch, uint8_t len, int32_t* data)
{
	char* ptr;
	uint8_t size;
	char temp[11];
	
	ptr= strnstr(src, ch, len);
	if(ptr== NULL)
		return NULL;
	
	size= ptr - src;
	memset(temp, 0, 11);
	memmove(temp, src, size);
	temp[size]= 0;
	
	*data= atoi(temp);
	
	return ptr;
}

uint32_t real_to_u32(float value)
{
	uint32_t u32_value;
	uint32_t * u32_value_ptr = (uint32_t*)(&value);

	
	u32_value= *(u32_value_ptr);
	
	return u32_value;
}
//////////////////

