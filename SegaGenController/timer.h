#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// NOTE: This relies on ACLK being set to ~12 kHz VLO during clock setup
void timerSetInterval(int intervalMillis);

// Timer millisecond counter.
// Bumped every time the timer fires; saturates at 0xFFFF.
// To be accessed only when interrupts are disabled.
extern volatile uint16_t g_timerMillisCounter;

#endif // TIMER_H
