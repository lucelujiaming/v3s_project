
#ifndef __JAG_H__
#define __JAG_H__

#include <stdint.h>

void JAG_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t JAG_Request(void);
uint8_t JAG_Analysis(uint16_t len);
#endif


