#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>

#define RADIO_REG_CONFIG      0x00
#define RADIO_REG_EN_AA       0x01
#define RADIO_REG_EN_RXADDR   0x02
#define RADIO_REG_SETUP_AW    0x03
#define RADIO_REG_SETUP_RETR  0x04
#define RADIO_REG_RF_CH       0x05
#define RADIO_REG_RF_SETUP    0x06
#define RADIO_REG_STATUS      0x07
#define RADIO_REG_OBSERVE_TX  0x08
#define RADIO_REG_RPD         0x09
#define RADIO_REG_RX_ADDR_P0  0x0A
#define RADIO_REG_RX_ADDR_P1  0x0B
#define RADIO_REG_RX_ADDR_P2  0x0C
#define RADIO_REG_RX_ADDR_P3  0x0D
#define RADIO_REG_RX_ADDR_P4  0x0E
#define RADIO_REG_RX_ADDR_P5  0x0F
#define RADIO_REG_TX_ADDR     0x10
#define RADIO_REG_RX_PW_P0    0x11
#define RADIO_REG_RX_PW_P1    0x12
#define RADIO_REG_RX_PW_P2    0x13
#define RADIO_REG_RX_PW_P3    0x14
#define RADIO_REG_RX_PW_P4    0x15
#define RADIO_REG_RX_PW_P5    0x16
#define RADIO_REG_FIFO_STATUS 0x17
#define RADIO_REG_DYNPD       0x1C
#define RADIO_REG_FEATURE     0x1D

uint8_t radioReadRegisterByte(uint8_t reg);
void radioWriteRegisterByte(uint8_t reg, uint8_t value);
void radioWriteRegister(uint8_t reg, uint8_t* data, int size);
void radioReadRXPayload(uint8_t* dest, int size);
void radioWriteTXPayload(uint8_t* src, int size);
void radioFlushTX();
void radioFlushRX();
void radioReuseTXPayload();
uint8_t radioGetRXPayloadWidth();
void radioWriteTXPayload(uint8_t* src, int size);
void radioNOP();
uint8_t radioReadStatus();

#endif // RADIO_H

