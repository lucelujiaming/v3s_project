#ifndef __CMB_SLAVE_H__
#define __CMB_SLAVE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "mbvardef.h"

bool check_crc(uint16_t frm_len);
void construc_frame_and_crc(void);
void frame_except(uint8_t except_code);

// uint8_t read_udp_holding_reg(uint16_t frm_len);
// uint8_t preset_udp_single_reg(uint16_t frm_len);
// uint8_t preset_udp_multi_reg(uint16_t frm_len);

// uint8_t read_rtu_holding_reg(uint16_t frm_len);
// uint8_t preset_rtu_single_reg(uint16_t frm_len);
// uint8_t preset_rtu_multi_reg(uint16_t frm_len);


uint16_t Modbus_GetFrameSizeToSend(void);
uint16_t ModbusUDP_FrameAnalysis(int16_t frm_len);
uint16_t ModbusRTU_FrameAnalysis(int16_t frm_len);
void Modbus_Init(uint8_t pt_addr, uint8_t* buff);

#endif
