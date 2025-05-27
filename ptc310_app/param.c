//param.c
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "eeprom.h"
#include "param.h"
#include "mbvardef.h" 

#include "config_operation.h"

#define   PARAM_CONFIG_FILE_NAME    "/root/app/param_config.ini"

void PARAM_Init(void)
{
	if(access(PARAM_CONFIG_FILE_NAME, F_OK) != 0) 
	{
		create_ram_in_inifile("HReg", 200, PARAM_CONFIG_FILE_NAME);
		// Use config of CP_DefaultValue
		PARAM_Save(0, 6);
		PARAM_Save(2, 20);
		PARAM_Save(3, 5);
		PARAM_Save(5, 3);
	}
	EE_Init();
}

void PARAM_Reload(uint16_t addr, uint16_t* buff, uint16_t len)
{
    char key[20]   = { 0 };
    char value[20] = { 0 };
	int  iSeq = 0;
	int iRet = 0;
	
    // 2. 打开PTC私有协议对应的串口
    for(int i = 0; i < len; i++) {
		sprintf(key, "Config%03d", addr + i);
        iRet = get_ini_key_string("HReg", key, value, PARAM_CONFIG_FILE_NAME);
	    if(iRet == 0)
	    {
		    buff[iSeq] = atoi(value);
	    }
		iSeq++;
    }
}

void PARAM_Save(uint16_t addr, int16_t value)
{
    char keyConfig[20]   = { 0 };
    char valueConfig[20] = { 0 };
	
	printf("PARAM_Save addr=%d and value = %d\n", addr, value);
	sprintf(keyConfig, "Config%03d", addr);
	sprintf(valueConfig, "%d", value);
    set_ini_key_string("HReg", keyConfig, valueConfig, PARAM_CONFIG_FILE_NAME);	
}
