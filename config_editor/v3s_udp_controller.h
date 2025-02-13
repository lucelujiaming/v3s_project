#ifndef __V3S_UDP_CONTROLLER_H__
#define __V3S_UDP_CONTROLLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 12345    // 服务器监听的端口号

#define MODBUS_CONFIG_GET_REQUEST           1 // 1 表示Modbus配置信息读取请求指令。
#define MODBUS_CONFIG_GET_RESPONSE          2 // 2 表示Modbus配置信息读取应答指令。
#define MODBUS_CONFIG_SET_REQUEST           3 // 3 表示Modbus配置信息修改请求指令。
#define MODBUS_CONFIG_SET_RESPONSE          4 // 4 表示Modbus配置信息修改应答指令。
#define INST_GET_LATEST_READINGS_REQUEST    5 // 5 表示获取最新仪表读数请求指令。
#define INST_GET_LATEST_READINGS_RESPONSE   6 // 6 表示获取最新仪表读数应答指令。

// Copy from MBVarDef.h
#define MODBUS_CONFIG_CP_PROTOCOL_ID				0
#define MODBUS_CONFIG_CP_INSTRUMENT_ADDR		    1
#define MODBUS_CONFIG_CP_ENQUIRY_TIME				2		// 0.1s
#define MODBUS_CONFIG_CP_RESPONSE_TIME		    	3		// 0.1s
#define MODBUS_CONFIG_CP_32BIT_LE					4		// Little Endian
#define MODBUS_CONFIG_CP_FAULT_TIMES				5
// 下面的是保留位。理论上不可以修改。
#define MODBUS_CONFIG_CP_RESERVED_NC1				6
#define MODBUS_CONFIG_CP_RESERVED_NC2				7
#define MODBUS_CONFIG_CP_FIRMWARE_VERSION			8
#define MODBUS_CONFIG_CP_REGISTER_ADDR_MAX	    	9

 #define MODBUS_CONFIG_RESPONSE_OK                  200    // 200 OK
 #define MODBUS_CONFIG_RESPONSE_NOCONTENT           204    // 204 No Content
 #define MODBUS_CONFIG_RESPONSE_BADREQUEST          400    // 400 Bad Request
 #define MODBUS_CONFIG_RESPONSE_FORBIDDEN           403    // 403 Forbidden
 #define MODBUS_CONFIG_RESPONSE_NOTFOUND            404    // 404 Not Found
 #define MODBUS_CONFIG_RESPONSE_INTERNAL_ERROR      500    // 500 Internal Server Error


int v3s_udp_control_server_init();

#endif
