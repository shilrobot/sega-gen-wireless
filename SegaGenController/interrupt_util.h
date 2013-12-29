#ifndef INTERRUPT_UTIL_H
#define INTERRUPT_UTIL_H

#include <msp430.h>

extern volatile uint8_t g_interruptSource;

#define INT_SRC_TIMER		0x1
#define INT_SRC_RADIO_IRQ	0x2

#define BEGIN_NO_INTERRUPTS \
	uint16_t old_sr = __get_SR_register(); \
	__disable_interrupt();

#define END_NO_INTERRUPTS \
	if (old_sr & BIT3) { \
		__enable_interrupt(); \
	}

#endif // INTERRUPT_UTIL_H
