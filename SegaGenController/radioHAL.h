#ifndef RADIOHAL_H
#define RADIOHAL_H

#include <msp430.h>

#define spiBegin() do { P1OUT &= ~BIT3; }while(0)
#define spiEnd() do { P1OUT |= BIT3; }while(0)

uint8_t spiTransfer(uint8_t data);

#endif /* RADIOHAL_H */
