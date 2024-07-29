#ifndef __USART_OPERATE_H__
#define __USART_OPERATE_H__

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"
#include "usart_type.h"

void Convert_USART_Send(int fd, uint16_t len);
uint16_t Convert_USART_FrameReceived(int fd);
uint8_t *Convert_USART_GetBuf();
void Convert_USART_SetRcvMode(USART_RCM_T mode, uint16_t par1, uint16_t par2);
uint8_t Convert_USART_SendComplete(int fd);

#endif

