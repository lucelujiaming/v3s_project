#include "v3s_udp_controller.h"

int v3s_udp_control_server_init()
{
    int sockfd;
    struct sockaddr_in server_addr;
 
    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }
 
    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // 接受任意IP地址
    server_addr.sin_port = htons(SERVER_PORT); // 端口号转换为网络字节序

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt));
	
    // 绑定套接字到指定的IP地址和端口
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("端口绑定失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("UDP服务器已启动，正在监听端口 %d...\n", SERVER_PORT);
	return sockfd;
}

