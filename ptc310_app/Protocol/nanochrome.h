
#ifndef __NANOCHROME_H__
#define __NANOCHROME_H__

#include <stdint.h>

void SERVOMEX_NANO_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t SERVOMEX_NANO_Request(void);
uint8_t SERVOMEX_NANO_Analysis(uint16_t len);
#endif


