
#ifndef __RELIYA_H__
#define __RELIYA_H__

#include <stdint.h>

// PMS_Init_1 for PDS-PA & HPGH-101
void RELIYA_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t RELIYA_HGPC_100_Analysis(uint16_t len);
#endif


