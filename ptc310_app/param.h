#ifndef __PARAM_H__
#define __PARAM_H__

#include <stdint.h>
#include <stdbool.h>
#include "eeprom.h"
#include "mbvardef.h"

void PARAM_Init(void);
void PARAM_Reload(uint16_t addr, uint16_t* buff, uint16_t len);
void PARAM_Save(uint16_t addr, int16_t value);

#endif
