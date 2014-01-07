#include "sleep.h"
#include "hal.h"
#include "tasks.h"
#include "awake.h"
#include "radio.h"

static void radioSleep()
{
    // Ensure radio is powered down
    radioWriteRegisterByte(RADIO_REG_CONFIG, 0);

    // Clear all queues
    radioFlushTX();
    radioFlushRX();
    radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);
}

static void sleepMode_onRadioIRQ()
{
    // Just make it go away
    radioFlushTX();
    radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);
}

static void sleepMode_onButtonChange()
{
    awakeMode_begin();
}

static int sleepMode_blah()
{
    P1OUT |= BIT6;
    halDelayMicroseconds(1000);
    P1OUT &= ~BIT6;

    return 0;
}

void sleepMode_begin()
{
    halBeginNoInterrupts();

    P1OUT &= ~BIT6;
    radioSleep();
    halSetTimerInterval(100, 1);
    halSetRadioIRQCallback(&sleepMode_onRadioIRQ);
    halSetButtonChangeCallback(&sleepMode_onButtonChange);
    clearTasks();
    //addTask(&sleepMode_blah, 250);

    halEndNoInterrupts();
}
