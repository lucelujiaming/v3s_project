#include <stdio.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "modbus.h"
#include "usart.h"

#include "Protocol/protocol.h"
#include "Protocol/Modbus/ModbusSlave.h"


//从站地址 17
#define SERVER_ID 17

#define MODBUS_RECV_UART_DEVICE     "COM1"
#define APP_SEND_UART_DEVICE        "/dev/ttyS3"

int open_and_new_rtu_slave(modbus_t* ctx_modbus, modbus_mapping_t* map, uint8_t pt_addr)
{
	int ret = 0;
    // 1. 打开Modbus口
	// 1.1. 设置串口信息
	ctx_modbus = modbus_new_rtu("COM1", 1200 * 16, 'N', 8, 1);
	if (NULL == ctx_modbus)
	{
		fprintf(stderr, "Error: %s\n", modbus_strerror(errno));
		return 1;
	}
	else
	{
		printf("设置串口信息成功\n");
	}

	// 1.2. 设置从机地址
	ret = modbus_set_slave(ctx_modbus, pt_addr);
	if (-1 == ret)
	{
		printf("设置从机地址失败.. %s\n", modbus_strerror(errno));
		modbus_free(ctx_modbus);
		return 1;
	}

	// 1.2.1 设置调试模式
	ret = modbus_set_debug(ctx_modbus, TRUE);
	if (-1 == ret)
	{
		printf("modbus_set_debug failed...\n");
		modbus_free(ctx_modbus);
		return 1;
	}

	// 1.3. 打开串口
	ret = modbus_connect(ctx_modbus);
	if (-1 == ret)
	{
		fprintf(stderr, "打开串口失败: %s\n", modbus_strerror(errno));
		modbus_free(ctx_modbus);
		return 1;
	}

	// 1.4. 申请内存 存放寄存器数据
	map = modbus_mapping_new(500, 500, 500, 500);
	if (NULL == map)
	{
		fprintf(stderr, "Error: mapping %s\n", modbus_strerror(errno));
		modbus_free(ctx_modbus);
		return 1;
	}
	return 0;
	
}

int close_and_free_rtu_slave(modbus_t* ctx_modbus, modbus_mapping_t* map)
{
	//6. 释放内存
	modbus_mapping_free(map);
	//7. 关闭设备
	modbus_close(ctx_modbus);
	modbus_free(ctx_modbus);
	return 0;
}


int open_ptc_port()
{
    int   fd; // , send_res;
    // 2. 打开PTC私有协议对应的串口
    printf("uart Start...\n");
    fd = open(APP_SEND_UART_DEVICE, O_RDWR|O_NOCTTY/*|O_NDELAY*/);
    if (fd < 0) {
        perror(APP_SEND_UART_DEVICE);
        exit(1);
    }
	return fd;
}


//RTU模式的Slave端程序
int main(void)
{
	int ret = 0;
    int   convert_protocol_fd; // , send_res;

	modbus_t* ctx_modbus_uart = NULL;
	modbus_mapping_t* map = NULL;

	uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
	ret = open_and_new_rtu_slave(ctx_modbus_uart, map, SERVER_ID);
	Modbus_Init(SERVER_ID, query);
	convert_protocol_fd = open_ptc_port();
    printf("uart Open...\n");
 
    // 2.1 设置串口参数
	Protocol_Init(convert_protocol_fd);
	// TimerInit();
	
	//5. 循环接受客户端请求，并且响应客户端
	while (1)
	{
		memset(serial_buff, 0, DATA_LENGTH);

		//获取查询请求报文
		ret = modbus_receive(ctx_modbus_uart, query);
		if (ret >= 0)
		{
			ret = Modbus_FrameAnalysis(ret);
			//恢复响应报文
			modbus_reply(ctx_modbus_uart, serial_buff, ret, map);
		}
		else
		{
			printf("Connection close\n");
		}
		Protocol_Proc(convert_protocol_fd);
		//////////////////////////////////////////////////////////////////////////
		HReg[HR_SW_VERSION]= FIRMWARE_VERSION;

		// Unit Reset
		if(HReg[HR_UNIT_RESET])
		{
			while(1)
			{
			}
		}
	}

	printf("Quit the loop: %s\n", modbus_strerror(errno));

	close_and_free_rtu_slave(ctx_modbus_uart, map);

    printf("uart Close...\n");
    close(convert_protocol_fd);
 
	return 0;
}

