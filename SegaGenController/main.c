#include "hal.h"
#include "radio.h"

typedef int (*TaskCallback)(void);

typedef struct _Task
{
    uint16_t intervalMillis;
    uint16_t millisLeft;
    TaskCallback callback;
} Task;

#define MAX_TASKS 4

static unsigned int g_numTasks;
static Task g_tasks[MAX_TASKS];

static uint8_t g_buttonState = 0;
static uint8_t g_inFlightState = 0;
static uint8_t g_receiverButtonState = 0;
static uint8_t g_everSent = 0;
static uint8_t g_sending = 0;

void clearTasks()
{
    g_numTasks = 0;
}

void addTask(TaskCallback cb, uint16_t interval)
{
    if (g_numTasks < MAX_TASKS)
    {
        Task* tsk = &g_tasks[g_numTasks];
        tsk->intervalMillis = interval;
        tsk->millisLeft = interval;
        tsk->callback = cb;
        g_numTasks++;
    }
}

void runTasks(uint16_t deltaMillis)
{
    unsigned int i;
    for (i=0; i < g_numTasks; ++i)
    {
        Task* task = &g_tasks[i];
        if (task->millisLeft <= deltaMillis)
        {
            if(task->callback())
            {
                // Terminate early.
                return;
            }

            task->millisLeft = task->intervalMillis;
        }
        else
        {
            task->millisLeft -= deltaMillis;
        }
    }
}

void radioWake()
{
    // CONFIG register
    // 7 Reserved       = 0
    // 6 MASK_RX_DR     = 0: enable RX interrupt
    // 5 MASK_TX_DR     = 0: enable TX complete interrupt
    // 4 MASK_MAX_RT    = 0: enable TX max retries interrupt
    // 3 EN_CRC         = 1: enable CRC
    // 2 CRC0           = 1: 2-byte CRC
    // 1 PWR_UP         = 1: power up
    // 0 PRIM_RX        = 0: primary TX
    radioWriteRegisterByte(RADIO_REG_CONFIG, BIT3 | BIT2 | BIT1);

    // Enable auto ack on pipes 0,1
    radioWriteRegisterByte(RADIO_REG_EN_AA, BIT1 | BIT0);

    // Enable receive on pipes 0,1
    radioWriteRegisterByte(RADIO_REG_EN_RXADDR, BIT1 | BIT0);

    // Set address width to 3 bytes
    radioWriteRegisterByte(RADIO_REG_SETUP_AW, 1);

    // Auto retransmit delay - 250 usec RX mode (MSByte=0), 0 auto retransmit (LSByte=0)
    radioWriteRegisterByte(RADIO_REG_SETUP_RETR, 0x00);

    // Set RF channel to 2403 MHz
    radioWriteRegisterByte(RADIO_REG_RF_CH, 3);

    // RF_SETUP
    // 7   CONT_WAVE    = 0: Continuous carrier transmit off (we are not in test mode)
    // 6   Reserved     = 0:
    // 5   RF_DR_LOW    = 0: With RF_DR_HIGH, set 1 Mbps rate
    // 4   PLL_LOCK     = 0: Do not force PLL lock (we are not in test mode)
    // 3   RF_DR_HIGH   = 0: With RF_DR_LOW, set 1 Mbps rate
    // 2:1 RF_PWR       = 11: 0 dBm
    // 0   Obsolete     = 0: (don't care)
    radioWriteRegisterByte(RADIO_REG_RF_SETUP, BIT2 | BIT1);

    uint8_t destAddr[] = {0xE7, 0xE7, 0xE7};
    uint8_t srcAddr[] = {0xC2, 0xC2, 0xC2};

    // Set TX/RX addresses. We must receive on the address we send to for auto-ACK to work.
    radioWriteRegister(RADIO_REG_RX_ADDR_P0, srcAddr, sizeof(srcAddr));
    radioWriteRegister(RADIO_REG_RX_ADDR_P1, destAddr, sizeof(destAddr));
    radioWriteRegister(RADIO_REG_TX_ADDR, destAddr, sizeof(destAddr));

    //radioWriteRegisterByte(RADIO_REG_RX_PW_P0, 32);

    // Enable dynamic payload length on pipes 0,1
    radioWriteRegisterByte(RADIO_REG_DYNPD, BIT1 | BIT0);

    // FEATURE
    // 2 EN_DPL         = 1: Enable dynamic payload length
    // 1 EN_ACK_PAY     = 0: Disable ACK payload
    // 0 EN_DYN_ACK     = 0: Don't need to send TX w/o ACK
    radioWriteRegisterByte(RADIO_REG_FEATURE, BIT2);

    // Clear all queues and clear interrupt bits
    radioFlushTX();
    radioFlushRX();
    radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);

    // Allow radio a good time to power up
    // (Tpd2stby)
    halDelayMicroseconds(5000UL);
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

void sendPacket()
{
    if (g_sending)
    {
        return;
    }

    g_sending = 1;
    g_inFlightState = g_buttonState;
    uint8_t buf[1] = {g_buttonState};

    radioWriteTXPayload(buf, sizeof(buf));

    halPulseRadioCE();
}

void sleepMode_begin();
void awakeMode_begin();

int sleepMode_pollButtons()
{
    //P1OUT ^= BIT6;

    uint8_t buttons = halReadButtons();
    if (buttons)
    {
        awakeMode_begin();
        return 1;
    }

    return 0;
}

void sleepMode_begin()
{
    halBeginNoInterrupts();

    radioSleep();
    halSetTimerInterval(250, 1);
    halSetRadioIRQCallback(0);
    clearTasks();
    addTask(&sleepMode_pollButtons, 250);

    halEndNoInterrupts();
}

void awakeMode_onButtonChange()
{
    g_buttonState = halReadButtons();

    if (!g_everSent || g_buttonState != g_receiverButtonState)
    {
        sendPacket();
    }

    if(g_buttonState)
    {
        P1OUT |= BIT6;
    }
    else
    {
        P1OUT &= ~BIT6;
    }
}

void awakeMode_onRadioIRQ()
{
    uint8_t status = radioReadStatus();

    uint8_t reTX = 0;

    // Max retransmissions hit
    if (status & BIT4)
    {
        radioFlushTX();
        g_sending = 0;
    }
    // Sent successfully
    else if (status & BIT5)
    {
        g_everSent = 1;
        g_receiverButtonState = g_inFlightState;
        g_sending = 0;
    }

    // Clear all interrupt bits
    radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);

    if (!g_everSent || g_buttonState != g_receiverButtonState)
    {
        sendPacket();
    }
}

int awakeMode_sendPacket()
{
    sendPacket();

    return 0;
}

void awakeMode_begin()
{
    halBeginNoInterrupts();

    radioWake();
    halSetTimerInterval(1, 10);
    halSetRadioIRQCallback(&awakeMode_onRadioIRQ);
    halSetButtonChangeCallback(&awakeMode_onButtonChange);
    clearTasks();
    addTask(&awakeMode_sendPacket, 1000);

    halEndNoInterrupts();
}

void initCB()
{
    // NOTE this is called w/ interrupts disabled

    halSetTimerCallback(&runTasks);
    awakeMode_begin();
}

int main(void)
{
    halMain(&initCB);
}
