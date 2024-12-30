//modbus_slave.c
#include <stdio.h>

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

    printf("check_crc error\n");
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
		printf("read_input_reg ILLEGAL_DATA_VALUE\n");
		return ILLEGAL_DATA_VALUE;
	}
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	points= _get_int16_int8_big_endian(&mb_data_buf[4]);

	bytes= points*2;

	if(bytes + 5 > 256)
	{
		printf("read_input_reg ILLEGAL_DATA_ADDR\n");
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
		printf("read_holding_reg ILLEGAL_DATA_VALUE\n");
		return ILLEGAL_DATA_VALUE;
	}
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	points= _get_int16_int8_big_endian(&mb_data_buf[4]);
	
	// printf("read_holding_reg reg_addr=%d and points=%d\n", reg_addr, points);
	bytes= points*2;

	if(bytes + 5 > 256)
	{
		printf("read_holding_reg ILLEGAL_DATA_ADDR\n");
		return ILLEGAL_DATA_ADDR;
	}

	mb_data_buf[2]= bytes;
	for(i= 0;i<points;i++)
	{
		if(reg_addr + i < HREG_MAX)
		{
			mb_data_buf[3+i*2]= HReg[reg_addr+i]>>8;
			mb_data_buf[4+i*2]= HReg[reg_addr+i]&0x00FF;
			// printf("read_holding_reg set %d to mb_data_buf\n", HReg[reg_addr+i]);
		}
		else
		{
			mb_data_buf[3+i*2]= 0;
			mb_data_buf[4+i*2]= 0;	
			// printf("read_holding_reg mb_data_buf=0\n");
		}
	}
	// 
	mb_frame_size= bytes+3;

		
	// printf("read_holding_reg mb_frame_size=%d\n", mb_frame_size);
	// for(int i = 0; i < mb_frame_size; i++)
	// {
	//  	printf("<%02X>", mb_data_buf[i]);
	// }
	// printf("\nData over\n");
	return 0;
}

uint8_t preset_single_reg(uint16_t frm_len)
{
	uint16_t reg_addr,value;
	
	reg_addr= _get_int16_int8_big_endian(&mb_data_buf[2]);
	printf("preset_single_reg reg_addr=%d\n", reg_addr);
	
	if(frm_len!= 8)
	{
		printf("preset_single_reg ILLEGAL_DATA_VALUE\n");
		return ILLEGAL_DATA_VALUE;
	}

	if(reg_addr < HREG_MAX)
	{
		value= _get_int16_int8_big_endian(&mb_data_buf[4]);
	    printf("preset_single_reg value=%d and HReg[reg_addr] = %d\n", value, HReg[reg_addr]);
		if(HReg[reg_addr]!= value)
		{
			HReg[reg_addr]= value;
			if(reg_addr >= CP_EEP_BASE && reg_addr <= CP_EEP_BASE + CP_EEP_MAX)
			{
	    		printf("preset_single_reg PARAM_Save value=%d and HReg[reg_addr] = %d\n", value, HReg[reg_addr]);
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
		printf("preset_multi_reg ILLEGAL_DATA_VALUE\n");
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
		    	printf("Modbus_FrameAnalysis CMD_READ_INPUT_REGISTER frm_len=%d\n", frm_len);
				exceptCode= read_input_reg(frm_len);
				break;
			case CMD_READ_HOLDING_REGISTER:
		    	// printf("Modbus_FrameAnalysis CMD_READ_HOLDING_REGISTER frm_len=%d\n", frm_len);
				exceptCode= read_holding_reg(frm_len);
				break;
			case CMD_PRESET_SINGLE_REGISTER:
		    	printf("Modbus_FrameAnalysis CMD_PRESET_SINGLE_REGISTER frm_len=%d\n", frm_len);
				exceptCode= preset_single_reg(frm_len);
				break;
			case CMD_PRESET_MULTIPLE_REGISTERS:
		    	printf("Modbus_FrameAnalysis CMD_PRESET_MULTIPLE_REGISTERS frm_len=%d\n", frm_len);
				exceptCode= preset_multi_reg(frm_len);
				break;
			default:
				printf("Modbus_FrameAnalysis ILLEGAL_FUNCTION\n");
				exceptCode= ILLEGAL_FUNCTION;
				break;
			}
	
			if(exceptCode)
			{
		    	printf("Modbus_FrameAnalysis frame_except exceptCode=%d\n", exceptCode);
				frame_except(exceptCode);
			}
			
			if(mb_data_buf[0]== MODBUS_BROADCAST_ADDR)
			{
		    	printf("Modbus_FrameAnalysis mb_frame_size=0\n");
				mb_frame_size= 0;
			}

			if(mb_frame_size)
			{
		    	// printf("Modbus_FrameAnalysis construc_frame_and_crc mb_frame_size=%d\n", mb_frame_size);
				construc_frame_and_crc();
			}
		}
	}
	else 
	{
		// printf("Error: mb_data_buf[0] check failed and mb_data_buf[0] is %d\n", mb_data_buf[0]);
		// printf("Start of Modbus_FrameAnalysis recv and frm_len is %d\n", frm_len);
		// for(int i = 0 ; i < frm_len; i++)
		// {
		//  	printf("<%02X> ", mb_data_buf[i]);
		// }
		// printf("\nEnd of Modbus_FrameAnalysis recv and frm_len is %d\n", frm_len);
		// usleep(2000);
		
		if((mb_data_buf[0] == 0xFE) && (frm_len > 1))
		{
			printf("We need jump over 0xFE and mb_data_buf[1] = %d\n", mb_data_buf[1]);
			if((mb_data_buf[1]== mb_addr || mb_data_buf[1]== MODBUS_BROADCAST_ADDR))
			{
				printf("Jump over 0xFE and mb_data_buf[1] = %d\n", mb_data_buf[1]);
				for(int i = 0; i < frm_len - 1; i++)
				{
					mb_data_buf[i] = mb_data_buf[i + 1];
				}
				printf("Call Modbus_FrameAnalysis and mb_data_buf[1] = %d\n", mb_data_buf[1]);
				return Modbus_FrameAnalysis(frm_len - 1);
			}
			printf("We need jump over 0xFE and mb_data_buf[1] = %d\n", mb_data_buf[1]);
		}
	}
	
	// printf("Modbus_FrameAnalysis return %d\n", mb_frame_size);
	return mb_frame_size;
}

void Modbus_Init(uint8_t pt_addr, uint8_t* buff) 
{
	mb_addr= pt_addr;
	mb_data_buf= buff;
}

