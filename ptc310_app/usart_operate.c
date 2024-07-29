#include <unistd.h>

#include "protocol.h"
#include "usart_operate.h"


// static volatile uint8_t DataBuff[256];
static uint8_t DataBuff[DATA_LENGTH];
// static volatile uint16_t send_len= 0;
// static volatile uint16_t send_count= 0;
// static volatile uint16_t recv_count= 0;
// static volatile uint8_t send_complete= 0;
// static volatile uint8_t interface;

static volatile uint8_t send_ret;

void Convert_USART_Send(int fd, uint16_t len)
{
    send_ret = write(fd, DataBuff, len);
}
uint16_t Convert_USART_FrameReceived(int fd)
{
    return read(fd, DataBuff, DATA_LENGTH);
}
uint8_t *Convert_USART_GetBuf()
{
    return DataBuff;
}

uint8_t Convert_USART_SendComplete(int fd)
{
    return send_ret;
}


