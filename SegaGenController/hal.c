#include "hal.h"

#define INT_SRC_TIMER		0x1
#define INT_SRC_RADIO_IRQ	0x2

static volatile uint8_t g_interruptSource = 0;
static volatile uint16_t g_timerMillisCounter = 0;
static TimerHandler g_timerCB = 0;
static EventHandler g_radioIRQCB = 0;
static int g_timerIntervalMillis = 0;

static void watchdogInit()
{
	// Stop watchdog timer
	// Supply password while writing WDTHOLD=1
	WDTCTL = WDTPW | WDTHOLD;
}

static void clockInit()
{
	// MCLK, SMCLK are 8 MHz calibrated DCO
	// ACLK is ~12 kHz VLO

	// BCSCTL2
	// MCLK comes from COCLK
	// MCLK divider is 1
	// SMCLK comes from DCOCLK
	// SMCLK divider is 1
	// Use internal DCO resistor (does this even apply?)
	BCSCTL2 = SELM_0 | DIVM_0;

	// DCOCTL
	// Set from calibrated 8 MHz settings
	DCOCTL = 0;
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	// BCSCTL1
	// Ensure XT2 oscillator is off
	BCSCTL1 |= XT2OFF;

	// BCSCTL3
	// Set ACLK to use VLO (~12 kHz)
	BCSCTL3 = LFXT1S_2 | XCAP_1;
}

static void gpioInit()
{
	// TODO: Set up all ports properly

	// P1.0: IRQ: Input; interrupt on falling edge
	// P1.1: MISO: Input (P1SEL & P1SEL2 set) - SPI
	// P1.2: MOSI: Output (P1SEL & P1SEL2 set) - SPI
	// P1.3: CSN: Output, initially HIGH
	// P1.4: SCK: Output (P1SEL & P1SEL2 set) - SPI
	// P1.5: CE: Output, initially LOW
	// P1.6: Green LED: Output
	// P1.7: Unused: Output, initially LOW

	P1DIR = BIT3 | BIT5 | BIT6 | BIT7;
	P1OUT = BIT3 | BIT6;
	P1SEL = BIT1 | BIT2 | BIT4;
	P1SEL2 = BIT1 | BIT2 | BIT4;
	P1IE = BIT0;
	P1IES = BIT0;

	// Port 2: buttons.
	// Initially all inputs, with pulldowns enabled.
	P2DIR = 00;
	P2OUT = 0;
	P2SEL = P2SEL2 = 0;
	P2REN = 0xFF;

	P3DIR = 0xFF;
	P3OUT = 0;
	P3SEL = P3SEL2 = 0;
}

static void spiInit()
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

static void radioInit()
{
	// Radio power-on reset time: 100 ms w/ 25% extra tolerance
	halDelayMicroseconds(125000);
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_HOOK(void)
{
	if (P1IFG & BIT0)
	{
		g_interruptSource |= INT_SRC_RADIO_IRQ;
		LPM3_EXIT;
	}

	P1IFG = 0;
}

uint8_t halReadButtons()
{
	// Turn on pull-up registers
	P2OUT = 0xFF;

	// Wait 1 usec
	halDelayMicroseconds(1);

	// Read keys
	uint8_t data = P2IN;

	// Change pull-ups to pull-downs
	P2OUT = 0x00;
	return ~data;
}

uint8_t halReadDIP()
{
	// TODO
	return 0;
}

uint16_t halReadBatteryVoltage()
{
	// TODO
	return 4200;
}

void halSetTimerInterval(int msec)
{
	halBeginNoInterrupts();

	g_timerIntervalMillis = msec;

	// Stop the timer, clear interrupt flag
	TA0CTL &= ~(MC1 | MC0);
	TA0CCTL0 &= ~CCIFG;

	// interrupt when CCR0 is reached.
	// TA0CCR0 = 12 * intervalMillis
	TA0CCR0 = (msec << 3) + (msec << 2);
	TA0CCTL0 = CM_0 | CCIE;
	TA0CTL = TASSEL_1 | ID_0 | MC_1;

	g_timerMillisCounter = 0;

	halEndNoInterrupts();
}

uint8_t halSpiTransfer(uint8_t data)
{
	UCA0TXBUF = data;

	while(!(IFG2 & UCA0RXIFG))
	{
		// spin :(
	}
	return UCA0RXBUF;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
	// READ VOLATILE
	uint16_t currentMillis = g_timerMillisCounter;

	uint16_t incrementedMillis = currentMillis + g_timerIntervalMillis;
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

void halSetTimerCallback(TimerHandler cb)
{
	g_timerCB = cb;
}

void halSetRadioIRQCallback(EventHandler cb)
{
	g_radioIRQCB = cb;
}

void halMain(EventHandler initCB)
{
	watchdogInit();
	clockInit();
	gpioInit();
	spiInit();
	radioInit();

	(initCB)();

	__enable_interrupt();

	while(1)
	{
		__disable_interrupt();

		uint8_t interruptSourceCopy = g_interruptSource;
		g_interruptSource = 0;
		uint16_t deltaMillis = g_timerMillisCounter;
		g_timerMillisCounter = 0;

		if (interruptSourceCopy == 0)
		{
			// Enter LPM3 with interrupts enabled
			_bis_SR_register(LPM3_bits | GIE);

			// ...Woke up from LPM3...
		}
		else
		{
			__enable_interrupt();

			if ((interruptSourceCopy & INT_SRC_TIMER) && g_timerCB)
			{
				(g_timerCB)(deltaMillis);
			}

			if ((interruptSourceCopy & INT_SRC_RADIO_IRQ) && g_radioIRQCB)
			{
				(g_radioIRQCB)();
			}
		}

		// TODO: ADC complete IRQ, etc.
	}
}
