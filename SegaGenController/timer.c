#include "timer.h"
#include "interrupt_util.h"
#include <msp430.h>

static volatile int timerIntervalMillis = 0;
volatile uint16_t g_timerMillisCounter = 0;

void timerSetInterval(int intervalMillis)
{
	BEGIN_NO_INTERRUPTS;

	timerIntervalMillis = intervalMillis;

	// Stop the timer, clear interrupt flag
	TA0CTL &= ~(MC1 | MC0);
	TA0CCTL0 &= ~CCIFG;

	// interrupt when CCR0 is reached.
	// TA0CCR0 = 12 * intervalMillis
	TA0CCR0 = (intervalMillis << 3) + (intervalMillis << 2);
	TA0CCTL0 = CM_0 | CCIE;
	TA0CTL = TASSEL_1 | ID_0 | MC_1;

	g_timerMillisCounter = 0;

	END_NO_INTERRUPTS;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
	// READ VOLATILE
	uint16_t currentMillis = g_timerMillisCounter;

	uint16_t incrementedMillis = currentMillis + timerIntervalMillis;
	if (incrementedMillis >= currentMillis)
	{
		g_timerMillisCounter = incrementedMillis;
	}
	else
	{
		// WRITE VOLATILE
		g_timerMillisCounter = 0xFFFF;
	}

	g_interruptSource |= INT_SRC_TIMER;
	LPM3_EXIT;
}
