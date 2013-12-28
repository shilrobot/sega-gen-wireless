#include <msp430.h> 
#include <stdint.h>
#include "radio.h"

#define CYCLES_PER_USEC 8

void setupWatchdog()
{
	// Stop watchdog timer
	// Supply password while writing WDTHOLD=1
	WDTCTL = WDTPW | WDTHOLD;
}

void setupClocks()
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

void setupGPIOs()
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

	P2DIR = 0xFF;
	P2OUT = 0;
	P2SEL = P2SEL2 = 0;

	P3DIR = 0xFF;
	P3OUT = 0;
	P3SEL = P3SEL2 = 0;
}

void setupTimer_A0()
{
	// 12 kHz timer, interrupt on overflow.
	// TODO: This value could be calibrated for room temp.?
	// (if it's WAY off)
//	TA0CCR0 = 12000 - 1;
	TA0CCR0 =  1200 - 1; // 10x / sec
	TA0CCTL0 = CM_0 | CCIE;
	TA0CTL = TASSEL_1 | ID_0 | MC_1;
}

void setupSPI()
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

void setupRadio()
{
	// Allow radio to start up, w/ 25% extra tolerance
	_delay_cycles(125000*CYCLES_PER_USEC);

	P1OUT |= BIT6;
	P1OUT &= ~BIT6;
	P1OUT |= BIT6;

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
	radioReadRegisterByte(RADIO_REG_CONFIG);

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

	// Clear all interrupt bits (paranoia)
	radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);

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

	radioFlushTX();
	radioFlushRX();
}

void setup()
{
	setupWatchdog();
	setupClocks();
	setupGPIOs();
	setupTimer_A0();
	setupSPI();
	setupRadio();

	__enable_interrupt();
}

#define FLAG_TIMER 		0x01
#define FLAG_RADIO_IRQ	0x02

volatile uint8_t flags = 0;

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void)
{
	P1OUT |= BIT6;
	flags |= FLAG_TIMER;
	LPM3_EXIT;
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_HOOK(void)
{
	P1OUT |= BIT6;
	P1IFG = 0;
	flags |= FLAG_RADIO_IRQ;
	LPM3_EXIT;
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

int main(void)
{
	setup();

	flags = 0;

	while(1)
	{
		P1OUT &= ~BIT6;
		LPM3;
		P1OUT |= BIT6;

		__disable_interrupt();
		uint8_t flagsCopy = flags;
		flags = 0;
		__enable_interrupt();

		if(flagsCopy & FLAG_TIMER)
		{
			sendPacket();
		}

		if(flagsCopy & FLAG_RADIO_IRQ)
		{
			onRadioIRQ();
		}
	}

	return 0;
}
