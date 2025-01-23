
#ifndef __PMS_H__
#define __PMS_H__

#include <stdint.h>

// PMS_Init_1 for PDS-PA & HPGH-101
void PMS_Init_1(int fd, uint16_t addr);
// PMS_Init_2 for Lasair III
void PMS_Init_2(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t PMS_PDS_PA_Analysis(uint16_t len);
uint16_t PMS_PDS_PA_DataColumnsNumber();
uint16_t PMS_PDS_PA_DataOutput(char* strOutput);

uint8_t PMS_HPGP_101_Analysis(uint16_t len);
uint16_t PMS_HPGP_101_DataColumnsNumber();
uint16_t PMS_HPGP_101_DataOutput(char* strOutput);

uint8_t PMS_LASAIR_III_Analysis(uint16_t len);
uint16_t PMS_LASAIR_III_DataColumnsNumber();
uint16_t PMS_LASAIR_III_DataOutput(char* strOutput);

uint8_t PMS_PDS_E_Analysis(uint16_t len);
uint16_t PMS_PDS_E_DataColumnsNumber();
uint16_t PMS_PDS_E_DataOutput(char* strOutput);

#endif


