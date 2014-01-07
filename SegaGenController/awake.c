#include "awake.h"
#include "tasks.h"
#include "hal.h"
#include "radio.h"
#include <string.h>

#define AWAKE_STATE_IDLE          0
#define AWAKE_STATE_SENDING       1
#define AWAKE_STATE_WAIT          2

typedef struct
{
    uint16_t stateMillis;
    uint16_t waitTime;
    uint8_t buttonState;
    uint8_t inFlightState;
    uint8_t receiverButtonState;
    uint8_t receiverButtonStateValid;
    uint8_t state;
    uint8_t consecutiveSendFailures;
} AwakeState;

static AwakeState g_awakeState;

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
    radioWriteRegister(RADIO_REG_RX_ADDR_P0, destAddr, sizeof(destAddr));
    radioWriteRegister(RADIO_REG_RX_ADDR_P1, srcAddr, sizeof(srcAddr));
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

void resendPacket()
{
    g_awakeState.state = AWAKE_STATE_SENDING;
    g_awakeState.stateMillis = 0;

    P1OUT |= BIT6;
    g_awakeState.inFlightState = g_awakeState.buttonState;
    uint8_t buf[1] = {g_awakeState.buttonState};

    radioWriteTXPayload(buf, sizeof(buf));
    halPulseRadioCE();
}

void sendPacket()
{
    g_awakeState.consecutiveSendFailures = 0;
    resendPacket();
}

void awakeMode_onButtonChange()
{
    g_awakeState.buttonState = halReadButtons();

    if ((g_awakeState.state == AWAKE_STATE_IDLE || g_awakeState.state == AWAKE_STATE_WAIT) &&
        (!g_awakeState.receiverButtonStateValid || g_awakeState.buttonState != g_awakeState.receiverButtonState))
    {
        sendPacket();
    }

//    if (g_awakeState.buttonState)
//    {
//        P1OUT |= BIT6;
//    }
//    else
//    {
//        P1OUT &= ~BIT6;
//    }
}

void awakeMode_onTXFailed()
{
    // We don't know what the receiver's state is because it may
    // have received the packet but lost the ACK.
    g_awakeState.receiverButtonStateValid = 0;

    if (g_awakeState.consecutiveSendFailures < 255)
    {
        g_awakeState.consecutiveSendFailures++;
    }

    if (g_awakeState.consecutiveSendFailures < 3)
    {
        resendPacket();
    }
    else if (g_awakeState.consecutiveSendFailures == 3)
    {
        g_awakeState.waitTime = 10;

        g_awakeState.state = AWAKE_STATE_WAIT;
        g_awakeState.stateMillis = 0;
    }
    else // > 3
    {
        g_awakeState.waitTime += g_awakeState.waitTime;
        if (g_awakeState.waitTime > 1000)
        {
            g_awakeState.waitTime = 1000;
        }

        g_awakeState.state = AWAKE_STATE_WAIT;
        g_awakeState.stateMillis = 0;
    }
}

void awakeMode_onTXSucceeded()
{
    g_awakeState.receiverButtonState = g_awakeState.inFlightState;
    g_awakeState.receiverButtonStateValid = 1;

    if (!g_awakeState.receiverButtonStateValid || g_awakeState.buttonState != g_awakeState.receiverButtonState)
    {
        sendPacket();
    }
    else
    {
        g_awakeState.state = AWAKE_STATE_IDLE;
        g_awakeState.stateMillis = 0;
    }
}

void awakeMode_onRadioIRQ()
{
    P1OUT &= ~BIT6;
    uint8_t status = radioReadStatus();

    // Max retransmissions hit (MAX_RT)
    if (status & BIT4)
    {
        // Flush TX buffer
        radioFlushTX();

        // Clear all interrupt bits
        radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);

        awakeMode_onTXFailed();
    }
    // Sent successfully (TX_DS)
    else if (status & BIT5)
    {
        // Clear all interrupt bits
        radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);

        awakeMode_onTXSucceeded();
    }
    else
    {
        // Some other interrupt (why?)
        // Clear all interrupt bits
        radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);
    }
}

int awakeMode_timerTick()
{
    g_awakeState.stateMillis += 10;

    if (g_awakeState.state == AWAKE_STATE_IDLE && g_awakeState.stateMillis > 1000)
    {
        sendPacket();
    }
    else if (g_awakeState.state == AWAKE_STATE_WAIT && g_awakeState.stateMillis >= g_awakeState.waitTime)
    {
        resendPacket();
    }

    return 0;
}

void awakeMode_begin()
{
    halBeginNoInterrupts();

    P1OUT &= ~BIT6;
    memset(&g_awakeState, 0, sizeof(g_awakeState));
    g_awakeState.buttonState = halReadButtons();
    g_awakeState.state = AWAKE_STATE_IDLE;

    radioWake();
    halSetTimerInterval(1, 10);
    halSetRadioIRQCallback(&awakeMode_onRadioIRQ);
    halSetButtonChangeCallback(&awakeMode_onButtonChange);
    clearTasks();
    addTask(&awakeMode_timerTick, 10);

    halEndNoInterrupts();
}
