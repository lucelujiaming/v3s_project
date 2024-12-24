#ifndef __USART_OPERATE_H__
#define __USART_OPERATE_H__

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"
#include "usart_type.h"

void Instrument_USART_Init();

void Instrument_USART_Send(int fd, uint16_t len);
uint16_t Instrument_USART_FrameReceived(int fd);
uint8_t *Instrument_USART_GetBuf();
uint8_t Instrument_USART_SendComplete(int fd);

#endif

