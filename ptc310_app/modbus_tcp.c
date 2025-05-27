#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <assert.h>

#include "modbus_tcp.h"

int modbus_tcp_server_init(int iPort, struct sockaddr_in * serverAddr)
{
	int modbus_fd = 0;
 
    // 创建UDP套接字
    modbus_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (modbus_fd < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }
 
    // 配置服务器地址
    memset(serverAddr, 0, sizeof(*serverAddr));
    serverAddr->sin_family = AF_INET; // IPv4
    serverAddr->sin_addr.s_addr = INADDR_ANY; // 接受任意IP地址
    serverAddr->sin_port = htons(iPort); // 端口号转换为网络字节序

	int opt = 1;
	setsockopt(modbus_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt));
	
    // 绑定套接字到指定的IP地址和端口
    if (bind(modbus_fd, (struct sockaddr *)serverAddr, sizeof(*serverAddr)) < 0) {
        perror("端口绑定失败");
        close(modbus_fd);
        exit(EXIT_FAILURE);
    }
	
    // 监听连接请求
    if (listen(modbus_fd, 3) < 0) {
        perror("Listen failed");
        close(modbus_fd);
        exit(EXIT_FAILURE);
    }
    printf("TCP服务器已启动，正在监听端口 %d...\n", iPort);
	
    return modbus_fd;
}

int modbus_tcp_server_uninit(int modbus_fd)
{
    if (modbus_fd != -1) {
        close(modbus_fd);
        modbus_fd = -1;
    }
	return 0;
}

