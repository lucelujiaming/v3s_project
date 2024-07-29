
#ifndef __SAES_H__
#define __SAES_H__

#include <stdint.h>

void SAES_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t SAES_Analysis(uint16_t len);
#endif


