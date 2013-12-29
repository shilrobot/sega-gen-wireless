#include "radio.h"
#include "spi.h"

uint8_t radioReadRegisterByte(uint8_t reg)
{
	spiBegin();
	spiTransfer(reg & 0x1F);
	uint8_t value = spiTransfer(0xFF);
	spiEnd();
	return value;
}

void radioWriteRegister(uint8_t reg, uint8_t* data, int size)
{
	spiBegin();
	spiTransfer(0x20 | (reg & 0x1F));
	for (; size > 0; --size, ++data)
	{
		spiTransfer(*data);
	}
	spiEnd();
}

void radioWriteRegisterByte(uint8_t reg, uint8_t value)
{
	spiBegin();
	spiTransfer(0x20 | (reg & 0x1F));
	spiTransfer(value);
	spiEnd();
}

void radioReadRXPayload(uint8_t* dest, int size)
{
	spiBegin();
	spiTransfer(0x61);
	for (; size > 0; --size, ++dest)
	{
		*dest = spiTransfer(0xFF);
	}
	spiEnd();
}

void radioWriteTXPayload(uint8_t* src, int size)
{
	spiBegin();
	spiTransfer(0xA0);
	for (; size > 0; --size, ++src)
	{
		spiTransfer(*src);
	}
	spiEnd();
}

void radioFlushTX()
{
	spiBegin();
	spiTransfer(0xE1);
	spiEnd();
}

void radioFlushRX()
{
	spiBegin();
	spiTransfer(0xE2);
	spiEnd();
}

void radioReuseTXPayload()
{
	spiBegin();
	spiTransfer(0xE3);
	spiEnd();
}

uint8_t radioGetRXPayloadWidth()
{
	spiBegin();
	spiTransfer(0x60);
	uint8_t value = spiTransfer(0xFF);
	spiEnd();
	return value;
}

void radioWriteTXPayloadNoACK(uint8_t* src, int size)
{
	spiBegin();
	spiTransfer(0xB0);
	for (; size > 0; --size, ++src)
	{
		spiTransfer(*src);
	}
	spiEnd();
}

void radioNOP()
{
	spiBegin();
	spiTransfer(0xFF);
	spiEnd();
}

uint8_t radioReadStatus()
{
	spiBegin();
	uint8_t status = spiTransfer(0xFF);
	spiEnd();
	return status;
}
