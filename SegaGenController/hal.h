#ifndef HAL_H
#define HAL_H

#include <msp430.h>
#include <stdint.h>

typedef void (*EventHandler)(void);
typedef void (*TimerHandler)(uint16_t deltaMillis);

void halMain(EventHandler initCB);

uint8_t halReadButtons();
uint8_t halReadDIP();
uint16_t halReadBatteryVoltage();

#define halSpiBegin() do { P1OUT &= ~BIT3; } while(0)
#define halSpiEnd() do { P1OUT |= BIT3; } while(0)

uint8_t halSpiTransfer(uint8_t mosi);

void halSetTimerInterval(int msec);
void halSetTimerCallback(TimerHandler cb);

void halSetRadioIRQCallback(EventHandler cb);

#define halDelayMicroseconds(usec) _delay_cycles((usec)*8)

#define halBeginNoInterrupts() \
	uint16_t halOldSR = __get_SR_register(); \
	__disable_interrupt();

#define halEndNoInterrupts() \
	if (halOldSR & BIT3) { \
		__enable_interrupt(); \
	}

#endif /* HAL_H */
