#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "v3s_udp_controller.h"

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

/**********************************************************************
 * 这个代码的输出策略是这样的。
 * 如果参数错误，打印提示信息。
 * 如果参数正确。要么打印正常的输出，要么打印错误码。
 **********************************************************************/
int main(int argc, char const *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char cReceiveBuffer[BUFFER_SIZE] = {0};
    char cSendBuffer[BUFFER_SIZE] = {0};
    socklen_t addr_len;
    
    if(argc < 2)
    {
        printf("[%s:%s:%d] ptc310_config_editor <GETMBC/GETMBJ/SETMB/GETLV> <PARAMS>\n", 
            __FILE__, __FUNCTION__, __LINE__);
        return 0;
    }
    else {
        if((strcmp(argv[1], "GETMBC") == 0) || (strcmp(argv[1], "GETMBJ") == 0))
        {
            ;
        }
        else if(strcmp(argv[1], "SETMB") == 0)
        {
            if(argc != 8)
            {
                printf("[%s:%s:%d] ptc310_config_editor SETMODBUS <DATA0> <DATA1> <DATA2> <DATA3> <DATA4> <DATA5>\n", 
                        __FILE__, __FUNCTION__, __LINE__);
                return 0;
            }
        }
        else if (strcmp(argv[1], "GETLV") == 0)
        {
            ;
        }
        else 
        {
            printf("[%s:%s:%d] ptc310_config_editor <GETMBC/GETMBJ/SETMB/GETLV> <PARAMS>\n",
                __FILE__, __FUNCTION__, __LINE__);
            return 0;
        }
    }
 
    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }
 
    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
 
    // 发送消息到服务器
    if((strcmp(argv[1], "GETMBC") == 0) || (strcmp(argv[1], "GETMBJ") == 0))
    {
        cSendBuffer[0] = MODBUS_CONFIG_GET_REQUEST;
        cSendBuffer[1] = 0x00;
        cSendBuffer[2] = 0x00;
        sendto(sockfd, cSendBuffer, 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        // 接收服务器的响应
        addr_len = sizeof(server_addr);
        ssize_t n = recvfrom(sockfd, cReceiveBuffer, BUFFER_SIZE, 0, 
                            (struct sockaddr *)&server_addr, &addr_len);
        if (n == MODBUS_CONFIG_CP_REGISTER_ADDR_MAX + 3) {
            if(strcmp(argv[1], "GETMBC") == 0)
            {
                for(int i = 0; i < MODBUS_CONFIG_CP_REGISTER_ADDR_MAX; i++)
				{
					printf("%d ", cReceiveBuffer[i + 3]);
				}
				printf("\n");
            }
            else if(strcmp(argv[1], "GETMBJ") == 0)
            {
				printf("{ ");
                for(int i = 0; i < MODBUS_CONFIG_CP_REGISTER_ADDR_MAX - 1; i++)
				{
					printf("\"Data%d\": %d, ", i, cReceiveBuffer[i + 3]);
				}
				printf("\"Data%d\": %d", MODBUS_CONFIG_CP_REGISTER_ADDR_MAX - 1, 
                    cReceiveBuffer[MODBUS_CONFIG_CP_REGISTER_ADDR_MAX + 2]);
				printf(" }\n");
            }
        }
        else 
		{
            printf("[%s:%s:%d] GETMBC/GETMBJ response length error: length is %d.\n",
                     __FILE__, __FUNCTION__, __LINE__, (int)n);
		}
    }
    else if(strcmp(argv[1], "GETLV") == 0)
    {
        cSendBuffer[0] = INST_GET_LATEST_READINGS_REQUEST;
        cSendBuffer[1] = 0x00;
        cSendBuffer[2] = 0x00;
        sendto(sockfd, cSendBuffer, 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        // 接收服务器的响应
        addr_len = sizeof(server_addr);
        ssize_t n = recvfrom(sockfd, cReceiveBuffer, BUFFER_SIZE, 0, 
                            (struct sockaddr *)&server_addr, &addr_len);
                            
        if (n > 4 + 3) {
            int iReadingNum = (n - 3) / 4;
            if(n != 3 + iReadingNum * 4)
            {
                printf("[%s:%s:%d] GETLV response length error: length is %d.\n",
                         __FILE__, __FUNCTION__, __LINE__, (int)n);
                return 1;
            }
            // printf("[%s:%s:%d] GETLV length is %d and iReadingNum length is %d.\n",
            //      __FILE__, __FUNCTION__, __LINE__, (int)n, iReadingNum);
			// printf("Start of GETLV return %d\n", (int)n);
			// for(int j = 0; j < (int)n; j++)
			// {
			// 		printf("<%02X> ", cReceiveBuffer[j]);
			// }
			// printf("\nEnd of GETLV return %d\n", (int)n);
            
            char cFloatBuffer[4] = {0};
			float* fReadingPtr = (float*)cFloatBuffer;
            for(int i = 0; i < iReadingNum - 1; i++)
            {
                memcpy(cFloatBuffer, &cReceiveBuffer[3 + i * 4], 4);
                printf("%.6f,", *fReadingPtr);
            }
            memcpy(cFloatBuffer, &cReceiveBuffer[3 + (iReadingNum - 1) * 4], 4);
            printf("%.6f", *fReadingPtr);
            printf("\n");
        }
    }
    else if(strcmp(argv[1], "SETMB") == 0)
    {
        cSendBuffer[0] = MODBUS_CONFIG_SET_REQUEST;
        cSendBuffer[1] = 0x00;
        cSendBuffer[2] = 0x06;
        cSendBuffer[3] = atoi(argv[2]);
        cSendBuffer[4] = atoi(argv[3]);
        cSendBuffer[5] = atoi(argv[4]);
        cSendBuffer[6] = atoi(argv[5]);
        cSendBuffer[7] = atoi(argv[6]);
        cSendBuffer[8] = atoi(argv[7]);
        sendto(sockfd, cSendBuffer, 9, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        // 接收服务器的响应
        addr_len = sizeof(server_addr);
        ssize_t n = recvfrom(sockfd, cReceiveBuffer, BUFFER_SIZE, 0, 
                            (struct sockaddr *)&server_addr, &addr_len);
        if (n == 5) {
            int iErrorCode = cReceiveBuffer[3] * 256 + cReceiveBuffer[3];
            // printf("[%s:%s:%d] SETMB response ErrorCode is %d.\n",
            //        __FILE__, __FUNCTION__, __LINE__, iErrorCode);
            printf("%d\n", iErrorCode);
        }
        else 
		{
            printf("[%s:%s:%d] SETMB response length error: length is %d.\n",
                     __FILE__, __FUNCTION__, __LINE__, (int)n);
		}
    }
 
    // 关闭套接字
    close(sockfd);
    return 0;
}

