// Version:1.0

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

uint32_t _abs_int32(int32_t data);
uint16_t _abs_int16(int16_t data);
float _abs_float(float data);

uint16_t _get_int16_int8_big_endian(uint8_t* pdata);
uint16_t _get_int16_int8_little_endian(uint8_t* pdata);
uint32_t _get_int32_int8_modbus(uint8_t* pdata);
uint32_t _get_int32_int16_modbus(uint16_t* pdata);

void _set_int16_int32_modbus(uint16_t* pdata,uint32_t data);
uint32_t _get_int32_int8_mbus(uint8_t* pdata);
void _set_int8_int32_mbus(uint8_t* pdata,uint32_t value);

uint16_t _swap_int16(uint16_t data);
uint8_t _swap_int8(uint8_t data);

uint8_t _high_byte_int16(uint16_t data);
uint8_t _low_byte_int16(uint16_t data);

uint16_t _high_word_int32(uint32_t data);
uint16_t _low_word_int32(uint32_t data);

uint16_t _BV16(uint8_t i);
uint32_t _BV32(uint8_t i);

void delay_ms(uint16_t time);

float analog_ramp(float pv, float set, float max_delta);
float hi_lo_limit(float pv, float hi, float lo);

float get_area_round(float diameter);

char* strnstr(char* src, char ch, uint8_t len);
char* str_int(char* src, char ch, uint8_t len, int32_t* data);

uint32_t real_to_u32(float value);

#endif
