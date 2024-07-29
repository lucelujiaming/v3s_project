//modbus_slave.c
#include "modbus.h"
#include "ModbusSlave.h"
#include "MBVarDef.h"
#include "eeprom.h"
#include "param.h"
#include "protocol.h"

extern uint16_t VirtAddVarTab[NumbOfVar];
uint8_t *mb_data_buf= NULL;

volatile uint8_t mb_frame_size;
static uint8_t mb_addr;

bool check_crc(uint16_t frm_len)
{
	uint16_t crc;
	
	crc= _crc16_modbus(mb_data_buf,frm_len-2);

	if((mb_data_buf[frm_len-1]== (uint8_t)(crc& 0x00FF)) 
		&& (mb_data_buf[frm_len-2]== (uint8_t)(crc>>8)))
	{
		return true;
	}

	return false;
}

void construc_frame_and_crc(void)
{
	uint16_t crc;

	crc= _crc16_modbus(mb_data_buf,mb_frame_size);
	mb_data_buf[mb_frame_size]= crc>>8;
	mb_data_buf[mb_frame_size+1]= crc & 0x00FF;
	mb_frame_size+= 2;
}

void frame_except(uint8_t except_code)
{
	mb_data_buf[1]|= 0x80;
	mb_data_buf[2]= except_code;
	mb_frame_size= 3;
}

uint16_t Modbus_GetFrameSizeToSend(void)
{
	uint16_t size;
	
	size= mb_frame_size;
	mb_frame_size= 0;

	return size;
}

uint8_t read_input_reg(uint16_t frm_len)
{
	uint16_t reg_addr,points;
	uint8_t bytes;
	uint8_t i;
	
	if(frm_len!= 8)
	{
		return ILLEGAL_DATA_VALUE;
	}
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	points= _get_int16_int8_big_endian(&mb_data_buf[4]);

	bytes= points*2;

	if(bytes + 5 > 256)
	{
		return ILLEGAL_DATA_ADDR;
	}

	mb_data_buf[2]= bytes;
	for(i= 0;i<points;i++)
	{
		if(reg_addr + i < IREG_MAX)
		{
			mb_data_buf[3+i*2]= IReg[reg_addr+i]>>8;
			mb_data_buf[4+i*2]= IReg[reg_addr+i]&0x00FF;
		}
		else
		{
			mb_data_buf[3+i*2]= 0;
			mb_data_buf[4+i*2]= 0;	
		}
	}
	mb_frame_size= bytes+3;
		
	return 0;
}

uint8_t read_holding_reg(uint16_t frm_len)
{
	uint16_t reg_addr,points;
	uint8_t bytes;
	uint8_t i;
	
	if(frm_len!= 8)
	{
		return ILLEGAL_DATA_VALUE;
	}
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	points= _get_int16_int8_big_endian(&mb_data_buf[4]);

	bytes= points*2;

	if(bytes + 5 > 256)
	{
		return ILLEGAL_DATA_ADDR;
	}

	mb_data_buf[2]= bytes;
	for(i= 0;i<points;i++)
	{
		if(reg_addr + i < HREG_MAX)
		{
			mb_data_buf[3+i*2]= HReg[reg_addr+i]>>8;
			mb_data_buf[4+i*2]= HReg[reg_addr+i]&0x00FF;
		}
		else
		{
			mb_data_buf[3+i*2]= 0;
			mb_data_buf[4+i*2]= 0;	
		}
	}
	mb_frame_size= bytes+3;
		
	return 0;
}

uint8_t preset_single_reg(uint16_t frm_len)
{
	uint16_t reg_addr,value;
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	
	if(frm_len!= 8)
	{
		return ILLEGAL_DATA_VALUE;
	}

	if(reg_addr < HREG_MAX)
	{
		value= _get_int16_int8_big_endian(&mb_data_buf[4]);
		if(HReg[reg_addr]!= value)
		{
			HReg[reg_addr]= value;
			if(reg_addr >= CP_EEP_BASE && reg_addr <= CP_EEP_BASE + CP_EEP_MAX)
			{
				PARAM_Save(reg_addr - CP_EEP_BASE, value); 
			}
		}
	}

	mb_frame_size= 6;
		
	return 0;
}

uint8_t preset_multi_reg(uint16_t frm_len)
{
	uint16_t reg_addr,points,value;
	uint8_t i;
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	points= _get_int16_int8_big_endian(&mb_data_buf[4]);
	
	if(frm_len - 9!= (uint8_t)points*2)
	{
		return ILLEGAL_DATA_VALUE;
	}

	for(i= 0;i<points;i++)
	{
		if(reg_addr + points <= HREG_MAX) 
		{
			value= _get_int16_int8_big_endian(&mb_data_buf[7+2*i]);
			if(HReg[reg_addr+i]!= value)
			{
				HReg[reg_addr+i]= value;
				if(reg_addr + i >= CP_EEP_BASE && reg_addr + i <= CP_EEP_BASE + CP_EEP_MAX)
				{
					PARAM_Save(reg_addr + i - CP_EEP_BASE, value);
				}
			}
		}
	}

	mb_frame_size= 6;
		
	return 0;
}

uint16_t Modbus_FrameAnalysis(int16_t frm_len)
{
	uint8_t exceptCode;
	
	mb_frame_size= 0;

	if(mb_data_buf[0]== mb_addr || mb_data_buf[0]== MODBUS_BROADCAST_ADDR)		//Identify adress
	{	
		if(check_crc(frm_len))
		{
			switch(mb_data_buf[1])
			{
			case CMD_READ_INPUT_REGISTER:
				exceptCode= read_input_reg(frm_len);
				break;
			case CMD_READ_HOLDING_REGISTER:
				exceptCode= read_holding_reg(frm_len);
				break;
			case CMD_PRESET_SINGLE_REGISTER:
				exceptCode= preset_single_reg(frm_len);
				break;
			case CMD_PRESET_MULTIPLE_REGISTERS:
				exceptCode= preset_multi_reg(frm_len);
				break;
			default:
				exceptCode= ILLEGAL_FUNCTION;
				break;
			}
	
			if(exceptCode)
			{
				frame_except(exceptCode);
			}
			
			if(mb_data_buf[0]== MODBUS_BROADCAST_ADDR)
			{
				mb_frame_size= 0;
			}

			if(mb_frame_size)
			{
				construc_frame_and_crc();
			}
		}
	}
	
	return mb_frame_size;
}

void Modbus_Init(uint8_t pt_addr, uint8_t* buff) 
{
	mb_addr= pt_addr;
	mb_data_buf= buff;
}

