#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include "SPI.h"

#define PIN_BTN 2
#define PIN_CE 8
#define PIN_IRQ 9
#define PIN_CSN 10
#define PIN_MOSI 11
#define PIN_MISO 12
#define PIN_SCK 13
#define PIN_LED 2

#define halSpiBegin() do { SPI.begin(); digitalWrite(PIN_CSN, LOW); } while(0)
#define halSpiEnd() do { SPI.end(); digitalWrite(PIN_CSN, HIGH); } while(0)
#define halSpiTransfer(_x) SPI.transfer(_x)

#endif /* HAL_H */
