#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <msp430.h>

#define spiBegin() do { P1OUT &= ~BIT3; }while(0)
#define spiEnd() do { P1OUT |= BIT3; }while(0)

uint8_t spiTransfer(uint8_t data);

void spiInit();

#endif // SPI_H
