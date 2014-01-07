#include "radio.h"
#include "hal.h"

uint8_t radioReadRegisterByte(uint8_t reg)
{
    halSpiBegin();
    halSpiTransfer(reg & 0x1F);
    uint8_t value = halSpiTransfer(0xFF);
    halSpiEnd();
    return value;
}

void radioWriteRegister(uint8_t reg, uint8_t* data, int size)
{
    halSpiBegin();
    halSpiTransfer(0x20 | (reg & 0x1F));
    for (; size > 0; --size, ++data)
    {
        halSpiTransfer(*data);
    }
    halSpiEnd();
}

void radioWriteRegisterByte(uint8_t reg, uint8_t value)
{
    halSpiBegin();
    halSpiTransfer(0x20 | (reg & 0x1F));
    halSpiTransfer(value);
    halSpiEnd();
}

void radioReadRXPayload(uint8_t* dest, int size)
{
    halSpiBegin();
    halSpiTransfer(0x61);
    for (; size > 0; --size, ++dest)
    {
        *dest = halSpiTransfer(0xFF);
    }
    halSpiEnd();
}

void radioWriteTXPayload(uint8_t* src, int size)
{
    halSpiBegin();
    halSpiTransfer(0xA0);
    for (; size > 0; --size, ++src)
    {
        halSpiTransfer(*src);
    }
    halSpiEnd();
}

void radioFlushTX()
{
    halSpiBegin();
    halSpiTransfer(0xE1);
    halSpiEnd();
}

void radioFlushRX()
{
    halSpiBegin();
    halSpiTransfer(0xE2);
    halSpiEnd();
}

void radioReuseTXPayload()
{
    halSpiBegin();
    halSpiTransfer(0xE3);
    halSpiEnd();
}

uint8_t radioGetRXPayloadWidth()
{
    halSpiBegin();
    halSpiTransfer(0x60);
    uint8_t value = halSpiTransfer(0xFF);
    halSpiEnd();
    return value;
}

void radioWriteTXPayloadNoACK(uint8_t* src, int size)
{
    halSpiBegin();
    halSpiTransfer(0xB0);
    for (; size > 0; --size, ++src)
    {
        halSpiTransfer(*src);
    }
    halSpiEnd();
}

void radioNOP()
{
    halSpiBegin();
    halSpiTransfer(0xFF);
    halSpiEnd();
}

uint8_t radioReadStatus()
{
    halSpiBegin();
    uint8_t status = halSpiTransfer(0xFF);
    halSpiEnd();
    return status;
}


