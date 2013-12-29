#include <msp430.h> 
#include <stdint.h>
#include <string.h>
#include "radio.h"
#include "spi.h"
#include "timer.h"
#include "interrupt_util.h"

#define CYCLES_PER_USEC 8

volatile uint8_t g_interruptSource = 0;

typedef void (*EventHandler)();
typedef void (*TimerHandler)(uint16_t delta);

typedef struct _Task
{
	uint16_t intervalMillis;
	uint16_t millisLeft;
	EventHandler callback;
} Task;

typedef struct _Mode
{
	EventHandler onEnter;
	EventHandler onRadioIRQ;
	EventHandler onExit;
	uint8_t numTasks;
	Task* tasks;
} Mode;

Mode* g_mode = 0;
Mode* g_nextMode = 0;

Task g_sleepModeTasks[1];
Mode g_sleepMode;

Task g_awakeModeTasks[1];
Mode g_awakeMode;

void watchdogInit()
{
	// Stop watchdog timer
	// Supply password while writing WDTHOLD=1
	WDTCTL = WDTPW | WDTHOLD;
}

void clockInit()
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

void gpioInit()
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

void radioInit()
{
	// Radio power-on reset time: 100 ms w/ 25% extra tolerance
	_delay_cycles(125000*CYCLES_PER_USEC);
}

void radioWake()
{
	// CONFIG register
	// 7 Reserved		= 0
	// 6 MASK_RX_DR 	= 0: enable RX interrupt
	// 5 MASK_TX_DR 	= 0: enable RX interrupt
	// 4 MASK_MAX_RT 	= 0: enable RX interrupt
	// 3 EN_CRC  		= 1: enable CRC
	// 2 CRC0    		= 1: 2-byte CRC
	// 1 PWR_UP  		= 1: power up
	// 0 PRIM_RX 		= 0: primary TX
	radioWriteRegisterByte(RADIO_REG_CONFIG, BIT3 | BIT2 | BIT1);

	// Enable auto ack on pipe 0
	radioWriteRegisterByte(RADIO_REG_EN_AA, BIT0);

	// Enable receive on pipe 0
	radioWriteRegisterByte(RADIO_REG_EN_RXADDR, BIT0);

	// Set address width to 3 bytes
	radioWriteRegisterByte(RADIO_REG_SETUP_AW, 1);

	// Auto retransmit delay - 250 usec RX mode (MSByte=0), 0 auto retransmit (LSByte=0)
	radioWriteRegisterByte(RADIO_REG_SETUP_RETR, 0x00);

	// Set RF channel to 2403 MHz
	radioWriteRegisterByte(RADIO_REG_RF_CH, 3);

	// RF_SETUP
	// 7   CONT_WAVE	= 0: Continuous carrier transmit off (we are not in test mode)
	// 6   Reserved		= 0:
	// 5   RF_DR_LOW	= 0: With RF_DR_HIGH, set 1 Mbps rate
	// 4   PLL_LOCK		= 0: Do not force PLL lock (we are not in test mode)
	// 3   RF_DR_HIGH	= 0: With RF_DR_LOW, set 1 Mbps rate
	// 2:1 RF_PWR		= 11: 0 dBm
	// 0   Obsolete		= 0: (don't care)
	radioWriteRegisterByte(RADIO_REG_RF_SETUP, BIT2 | BIT1);

	uint8_t destAddr[] = {0x12,0x34,0x56};

	// Set TX/RX addresses. We must receive on the address we send to for auto-ACK to work.
	radioWriteRegister(RADIO_REG_RX_ADDR_P0, destAddr, sizeof(destAddr));
	radioWriteRegister(RADIO_REG_TX_ADDR, destAddr, sizeof(destAddr));

	radioWriteRegisterByte(RADIO_REG_RX_PW_P0, 32);

	// Enable dynamic payload length on pipe 0
	radioWriteRegisterByte(RADIO_REG_DYNPD, BIT0);

	// FEATURE
	// 2 EN_DPL 		= 1: Enable dynamic payload length
	// 1 EN_ACK_PAY 	= 0: Disable ACK payload
	// 0 EN_DYN_ACK 	= 0: Don't need to send TX w/o ACK
	radioWriteRegisterByte(RADIO_REG_FEATURE, BIT2);

	// Clear all queues and clear interrupt bits
	radioFlushTX();
	radioFlushRX();
	radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);

	// Allow radio a good time to power up
	// (Tpd2stby)
	_delay_cycles(5000UL*CYCLES_PER_USEC);
}

void radioSleep()
{
	// Ensure radio is powered down
	radioWriteRegisterByte(RADIO_REG_CONFIG, 0);

	// Clear all queues
	radioFlushTX();
	radioFlushRX();
	radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);
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

void sendPacket()
{
	uint8_t buf[] = {0xAA};

	radioWriteTXPayload(buf, sizeof(buf));

	// Pulse CE
	// CE LOW
	P1OUT |= BIT5;
	_delay_cycles(13*CYCLES_PER_USEC); // 10us min pulse width + 25% buffer
	// CE HIGH
	P1OUT &= ~BIT5;
}

void onRadioIRQ()
{
	uint8_t status = radioReadStatus();
	radioReadRegisterByte(RADIO_REG_OBSERVE_TX);

	if (status & BIT4)
	{
		radioFlushTX();
	}

	// Clear all interrupt bits
	radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);
}

uint8_t sampleButtons()
{
	// Turn on pull-up registers
	P2OUT = 0xFF;

	// Wait 1 usec
	__delay_cycles(1*CYCLES_PER_USEC);

	// Read keys
	uint8_t data = P2IN;

	// Change pull-ups to pull-downs
	P2OUT = 0x00;
	return ~data;
}

void processModeSwitch()
{
	if (g_nextMode == 0)
	{
		return;
	}

	if (g_mode != 0 && g_mode->onExit != 0)
	{
		g_mode->onExit();
	}

	g_mode = g_nextMode;
	g_nextMode = 0;

	unsigned int i;
	for (i=0; i<g_mode->numTasks; ++i)
	{
		g_mode->tasks[i].millisLeft = g_mode->tasks[i].intervalMillis;
	}

	if (g_mode->onEnter != 0)
	{
		(g_mode->onEnter)();
	}
}

void sleepMode_onEnter()
{
	BEGIN_NO_INTERRUPTS;

	radioSleep();
	timerSetInterval(250);

	END_NO_INTERRUPTS;
}

void sleepMode_pollButtons()
{
	P1OUT ^= BIT6;

	uint8_t buttons = sampleButtons();
	if (buttons)
	{
		g_nextMode = &g_awakeMode;
	}
}

void awakeMode_onEnter()
{
	BEGIN_NO_INTERRUPTS;

	radioWake();
	timerSetInterval(2);

	END_NO_INTERRUPTS;
}

void awakeMode_pollButtons()
{
	P1OUT ^= BIT6;
	uint8_t buttons = sampleButtons();
	if (!buttons)
	{
		g_nextMode = &g_sleepMode;
	}
//	if(buttons)
//		P1OUT |= BIT6;
//	else
//		P1OUT &= ~BIT6;
}

void awakeMode_onRadioIRQ(uint16_t millis)
{

}

void init()
{
	watchdogInit();
	clockInit();
	gpioInit();
	spiInit();
	radioInit();

	g_sleepMode.onEnter = &sleepMode_onEnter;
	g_sleepMode.numTasks = 1;
	g_sleepMode.tasks = &g_sleepModeTasks[0];
	g_sleepModeTasks[0].intervalMillis = 250;
	g_sleepModeTasks[0].callback = &sleepMode_pollButtons;

	g_awakeMode.onEnter = &awakeMode_onEnter;
	g_awakeMode.onRadioIRQ = &awakeMode_onRadioIRQ;
	g_awakeMode.numTasks = 1;
	g_awakeMode.tasks = &g_awakeModeTasks[0];
	g_awakeModeTasks[0].intervalMillis = 100;
	g_awakeModeTasks[0].callback = &awakeMode_pollButtons;

	g_nextMode = &g_sleepMode;
	processModeSwitch();

	__enable_interrupt();
}

int main(void)
{
	unsigned int i;

	init();

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

			if (interruptSourceCopy & INT_SRC_TIMER)
			{
				// Update timer tasks
				for (i = 0; i <g_mode->numTasks; ++i)
				{
					Task* tsk = &(g_mode->tasks[i]);
					if (tsk->millisLeft <= deltaMillis)
					{
						tsk->millisLeft = tsk->intervalMillis;
						(tsk->callback)();

						if (g_nextMode)
						{
							break;
						}
					}
					else
					{
						tsk->millisLeft -= deltaMillis;
					}
				}

				if (g_nextMode)
				{
					processModeSwitch();
				}
			}

			if ((interruptSourceCopy & INT_SRC_RADIO_IRQ) &&
				g_mode->onRadioIRQ != 0)
			{
				(g_mode->onRadioIRQ)();

				if (g_nextMode)
				{
					processModeSwitch();
				}
			}
		}

		// TODO: ADC complete IRQ, etc.
	}
}
