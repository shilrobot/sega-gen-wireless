#include "spi.h"

void spiInit()
{
	UCA0CTL1 = UCSWRST;

	// CPOL = 0
	// CPHA = 0
	UCA0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;
	UCA0CTL1 |= UCSSEL_2;
	UCA0BR0 = 2; // 8 MHz clock / 2 = 4 MHz SPI
	UCA0BR1 = 0;

	UCA0CTL1 &= ~UCSWRST;
}

uint8_t spiTransfer(uint8_t data)
{
	UCA0TXBUF = data;

	while(!(IFG2 & UCA0RXIFG))
	{
		// spin :(
	}
	return UCA0RXBUF;
}
