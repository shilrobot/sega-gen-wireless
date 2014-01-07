#include "hal.h"

// Interrupt source (written to by ISRs to wake up main thread)
#define INT_SRC_BUTTON_CHANGE   0x1
#define INT_SRC_TIMER           0x2
#define INT_SRC_RADIO_IRQ       0x4

static volatile uint8_t g_interruptSource = 0;

// Timer configuration
static int g_timerDivider = 0;
static int g_timerTickInterval = 0;

// Timer tracking
static int g_timerDivCounter = 0;
static volatile uint16_t g_timerMillisCounter = 0;

// Key change detection
// Goes along with an INT_SRC_BUTTON_CHANGE to notify us of what the IRQ saw
static volatile uint8_t g_lastButtons = 0;
// Our captured copy of this (updated once per "main thread" interrupt processing cycle)
static uint8_t g_lastButtonsCapture = 0;

// Callbacks to higher layer
static TimerHandler g_timerCB = 0;
static EventHandler g_radioIRQCB = 0;
static EventHandler g_buttonsCB = 0;

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
    // Note: we especially want to set XCAP=00 so no extra
    // capacitance is seen on the button pins.
    BCSCTL3 = LFXT1S_2;
}

static void gpioInit()
{
    // TODO: Set up all ports properly for final config.

    // Port 1
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

    // Port 2: Gamepad buttons
    // Initially all inputs, with pulldowns enabled.
    P2DIR = 0;
    P2OUT = 0;
    P2SEL = P2SEL2 = 0;
    P2REN = 0xFF;

    // Port 3
    // P3.0: CHAN0: Input
    // P3.1: CHAN1: Input
    // P3.2: LED: Output (PWM)
    // P3.3: CHAN2: Input
    // P3.4: CHAN3: Input
    // P3.5: NC - output low
    // P3.6: NC - output low
    // P3.7: NC - output low
    // CHANx all have pulldowns enabled by default
    P3DIR = BIT2 | BIT5 | BIT6 | BIT7;
    P3OUT = 0;
    P3SEL = P3SEL2 = 0;
    P3REN = BIT0 | BIT1 | BIT3 | BIT4;
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
    return g_lastButtonsCapture;
}

uint8_t halReadDIP()
{
    // Turn on pull-up registers
    P3OUT = BIT0 | BIT1 | BIT3 | BIT4;

    // Wait 1 usec for port capacitance to charge through pullups
    halDelayMicroseconds(2);

    // Read keys
    uint8_t data = P3IN;

    // Change pull-ups back to pull-downs
    P3OUT &= ~(BIT0 | BIT1 | BIT3 | BIT4);

    // Compute final result.
    // Must invert bits and then do some shifting.
    data = ~data;
    return (data & 0b11) | ((data & 0b11000) >> 3);
}

uint16_t halReadBatteryVoltage()
{
    // TODO: This.
    return 4200;
}

void halPulseRadioCE()
{
    P1OUT |= BIT5;
    halDelayMicroseconds(13); // 10us min pulse width + 25% buffer
    // CE HIGH
    P1OUT &= ~BIT5;
}

void halSetTimerInterval(int keyPollInterval, int divider)
{
    halBeginNoInterrupts();

    g_timerDivider = divider;
    g_timerDivCounter = divider;
    g_timerTickInterval = keyPollInterval * divider;

    // Stop the timer, clear interrupt flag
    TA0CTL &= ~(MC1 | MC0);
    TA0CCTL0 &= ~CCIFG;

    // interrupt when CCR0 is reached.
    // TA0CCR0 = 12 * intervalMillis
    TA0CCR0 = (keyPollInterval << 3) + (keyPollInterval << 2);
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
    // Turn on pull-up registers
    P2OUT = 0xFF;

    // Wait 2 usec for port capacitance to charge through pullups
    // (6 us will do fine. This gives us a lot of margin, without
    // taking TOO much extra time.)
    halDelayMicroseconds(2);

    // Read keys
    uint8_t buttons = ~P2IN;

    // Change pull-ups to pull-downs
    P2OUT = 0x00;

    if (buttons != g_lastButtons)
    {
        g_lastButtons = buttons;
        g_interruptSource |= INT_SRC_BUTTON_CHANGE;
        LPM3_EXIT;
        // note, this doesn't return! it keeps going to the next bit.
    }

    // Handle timer (divided down from key polling interval)
    g_timerDivCounter--;
    if (g_timerDivCounter <= 0)
    {
        g_timerDivCounter = g_timerDivider;

        // READ VOLATILE
        uint16_t currentMillis = g_timerMillisCounter;

        uint16_t incrementedMillis = currentMillis + g_timerTickInterval;
        if (incrementedMillis >= currentMillis)
        {
            // WRITE VOLATILE
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
}

static void nullTimerHandler(uint16_t ignored)
{
}

static void nullHandler()
{

}

void halSetTimerCallback(TimerHandler cb)
{
    g_timerCB = cb ? cb : nullTimerHandler;
}

void halSetRadioIRQCallback(EventHandler cb)
{
    g_radioIRQCB = cb ? cb : nullHandler;
}

void halSetButtonChangeCallback(EventHandler cb)
{
    g_buttonsCB = cb ? cb : nullHandler;
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
        uint16_t deltaMillis = g_timerMillisCounter;
        g_lastButtonsCapture = g_lastButtons;
        g_interruptSource = 0;
        g_timerMillisCounter = 0;

        if (interruptSourceCopy)
        {
            __enable_interrupt();

            if (interruptSourceCopy & INT_SRC_BUTTON_CHANGE)
            {
                (g_buttonsCB)();
            }

            if (interruptSourceCopy & INT_SRC_TIMER)
            {
                (g_timerCB)(deltaMillis);
            }

            if (interruptSourceCopy & INT_SRC_RADIO_IRQ)
            {
                (g_radioIRQCB)();
            }

        }
        else
        {
            // Enter LPM3 with interrupts enabled
            _bis_SR_register(LPM3_bits | GIE);

            // ...Woke up from LPM3...
        }

        // TODO: ADC complete IRQ, etc.
    }
}
