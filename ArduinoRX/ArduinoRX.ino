#include "radio.h"
#include "hal.h"
#include "SPI.h"

#define BIT0 (1<<0)
#define BIT1 (1<<1)
#define BIT2 (1<<2)
#define BIT3 (1<<3)
#define BIT4 (1<<4)
#define BIT5 (1<<5)
#define BIT6 (1<<6)
#define BIT7 (1<<7)

void radioSetup()
{
  // Wait for radio to enter power-down state
  delay(125);
  
  // CONFIG register
  // 7 Reserved       = 0
  // 6 MASK_RX_DR     = 0: enable RX interrupt
  // 5 MASK_TX_DR     = 0: enable TX complete interrupt
  // 4 MASK_MAX_RT    = 0: enable TX max retries interrupt
  // 3 EN_CRC         = 1: enable CRC
  // 2 CRC0           = 1: 2-byte CRC
  // 1 PWR_UP         = 1: power up
  // 1 PRIM_RX        = 1: primary RX
  radioWriteRegisterByte(RADIO_REG_CONFIG, BIT3 | BIT2 | BIT1 | BIT0);
  
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
  
  uint8_t destAddr[] = {0xC2, 0xC2, 0xC2};
  uint8_t srcAddr[] = {0xE7, 0xE7, 0xE7};
  
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
    
  // Go from power-down to power-up
  delay(5);
  
  // CE high to receive
  digitalWrite(PIN_CE, HIGH);
}

void setup()
{
  digitalWrite(PIN_CE, LOW);
  digitalWrite(PIN_CSN, HIGH);
  digitalWrite(PIN_MOSI, LOW);
  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_LED, LOW);
  
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_CE, OUTPUT);
  pinMode(PIN_CSN, OUTPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_IRQ, INPUT); 
  pinMode(PIN_MISO, INPUT);
  
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  
  Serial.begin(115200);
  
  radioSetup();
}

void handleRX_DR()
{
  while(1)
  {        
    // How big is it?
    uint8_t packetSize = radioGetRXPayloadWidth();
    
    if (packetSize == 0 || packetSize > 32)
    {
      Serial.print("Bad packet size: ");
      Serial.print(packetSize);
      Serial.print("\n");
      radioFlushRX();
      radioWriteRegisterByte(RADIO_REG_STATUS, _BV(6));
      return;
    }
  
    uint8_t packet[32];
    radioReadRXPayload(&packet[0], packetSize);
    
    digitalWrite(PIN_LED, packet[0] ? HIGH : LOW);
    
    // Dump packet
    Serial.print("Packet size: ");
    Serial.print(packetSize);
    Serial.print("\n");
    for (int i=0; i<packetSize; ++i) {
      Serial.print(packet[i], HEX);
      Serial.print(",");
    }
    Serial.print("\n\n");
    
    
    // Clear the RX_DR IRQ
    radioWriteRegisterByte(RADIO_REG_STATUS, _BV(6));
    
    // Check if there are more packets to read
    uint8_t fifoStatus = radioReadRegisterByte(RADIO_REG_FIFO_STATUS);
    if (fifoStatus & _BV(0))
    {
      // RX_EMPTY. We're done
      return;
    }
    else
    {
      continue;
    }
  }
}

void loop()
{
    if(!digitalRead(PIN_IRQ))
    {
      uint8_t status = radioReadStatus();
      
      // RX_DR interrupt
      if (status & _BV(6))
      {
        handleRX_DR();
      }
      else
      {
        // clear ALL interrupts -- we somehow transmitted something?
        radioWriteRegisterByte(RADIO_REG_STATUS, _BV(6) | _BV(5) | _BV(4));
      }
    }
}

