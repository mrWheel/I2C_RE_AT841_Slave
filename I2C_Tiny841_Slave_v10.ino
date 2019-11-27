/*
    Program : I2C_Tiny841_Slave
    Date    : 10-09-2019
*/
#define _MAJOR_VERSION  1
#define _MINOR_VERSION  0
/*
    Copyright (C) 2019 Willem Aandewiel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
* You need https://github.com/SpenceKonde/ATTinyCore to compile this source
*
* Settings:
*    Board:  ATtiny 441/841 (Optiboot)
*    Chip:   Attiny841
*    Clock:  8 MHz (internal)
*    B.O.D. Level: B.O.D. Enabled (1.8v)
*    B.O.D. Mode (active): B.O.D. Disabled
*    B.O.D. Mode (sleep): B.O.D. Disabled
*    Save EEPROM: EEPROM retained
*    Pin Mapping: Clockwise (like Rev. D boards)
*    LTO (1.611+ only): Enabled
*    Bootloader UART: UART0 (Serial)
*    Wire Modes: Slave Only
*    Port A (CW:0~7, CCW:3~10)
*    millis()/micros(): Enabled
*/

#include <avr/wdt.h>
#include <Wire.h>
#include <EEPROM.h>

// Nico Hood's library: https://github.com/NicoHood/PinChangeInterrupt/
// Used for pin change interrupts on ATtinys
// Note: To make this code work with Nico's library you have to comment out
// https://github.com/NicoHood/PinChangeInterrupt/blob/master/src/PinChangeInterruptSettings.h#L228
#include "PinChangeInterrupt.h"

#define _I2C_DEFAULT_ADDRESS  0x28

#define _ROTA           10          // DIL-2  (PB0, D10)
#define _ROTB           0           // DIL-13  (PA0, D0) 

#define _BUTTON         2           // DIL-11  (PA2, D2)
#define _BLUELED        5           // DIL-8  (PA5, D5)
#define _REDLED         8           // DIL-5 (PB2, D8)
#define _GRNLED         7           // DIL-6 (PA7, D7)
#define _SEND_INT       1           // DIL-12  (PA1, D1)

#define _LED_ON         0
#define _LED_OFF        255

struct registerLayout {
  byte      status;         // 0x00
  byte      address;        // 0x01
  byte      majorRelease;   // 0x02
  byte      minorRelease;   // 0x03
  int16_t   rotVal;         // 0x04 - 2 bytes
  int16_t   rotStep;        // 0x06 - 2 bytes
  int16_t   rotMin;         // 0x08 - 2 bytes
  int16_t   rotMax;         // 0x0A - 2 bytes
  uint8_t   rotSpinTime;    // 0x0C
  byte      pwmRed;         // 0x0D
  byte      pwmGreen;       // 0x0E
  byte      pwmBlue;        // 0x0F
  uint8_t   debounceTime;   // 0x10
  uint16_t  midPressTime;   // 0x11 - 2 bytes
  uint16_t  longPressTime;  // 0x13 - 2 bytes
  uint8_t   modeSettings;   // 0x15 -->> _MODESETTINGS must be the same offset <<--
  byte      filler[4];      // 0x16
};

#define _MODESETTINGS   0x15
#define _CMD_REGISTER   0xF0

//These are the defaults for all settings
volatile registerLayout registerStack = {
  .status =                     0,    // 0x00
  .address = _I2C_DEFAULT_ADDRESS,    // 0x01
  .majorRelease =  _MAJOR_VERSION,    // 0x02
  .minorRelease =  _MINOR_VERSION,    // 0203
  .rotVal =                     0,    // 0x04 2
  .rotStep =                    1,    // 0x06 2
  .rotMin =                     0,    // 0x08 2
  .rotMax =                   255,    // 0x0A 2
  .rotSpinTime =               10,    // 0x0C
  .pwmRed =                     0,    // 0x0D
  .pwmGreen =                   0,    // 0x0E
  .pwmBlue =                    0,    // 0x0F
  .debounceTime =              10,    // 0x10
  .midPressTime =             500,    // 0x11 2
  .longPressTime =           1500,    // 0x13 2
  .modeSettings =               0,    // 0x15
  .filler =  {0xFF, 0xFF, 0xFF,0xFF}  // 0x16
};

//Cast 32bit address of the object registerStack with uint8_t so we can increment the pointer
uint8_t *registerPointer = (uint8_t *)&registerStack;

volatile byte registerNumber; 

//------ status bit's -------------------------------------------------
enum  {  INTERRUPT_BIT
       , COUNTUP_BIT, COUNTDOWN_BIT
       , PRESSED_BIT, QUICKRELEASE_BIT, MIDRELEASE_BIT, LONGRELEASE_BIT
      };
//------ commands ----------------------------------------------------------
enum  {  CMD_READCONF, CMD_WRITECONF, CMD_DUM2, CMD_DUM3, CMD_DUM4, CMD_DUM5
       , CMD_DUM6,  CMD_REBOOT
      };
//------ Setting bits --------------------------------------------------------
enum  {  STNG_HWROTDIR, STNG_FLIPMODE, STNG_TURNMODE, STNG_SPARE3, STNG_SPARE4
       , STNG_SPARE5, STNG_SPARE_6, STNG_SPARE7
      };
//------ finite states ---------------------------------------------------------------
enum  {  BTN_INIT, BTN_FIRST_PRESS, BTN_IS_PRESSED, BTN_FIRST_RELEASE, BTN_IS_RELEASED };


uint8_t btnState = 0;

static uint8_t    prevNextCode = 0;
static uint16_t   store=0;
static int8_t     rot_enc_table[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};
int16_t           prevRotaryPos;
volatile int8_t   hwRotDir = 1,     modeRotDir  = 1;
bool              buttonPrev        = HIGH;
bool              buttonState       = HIGH;
bool              pressedStateSend  = false;
byte              midPressActive    = 0;
byte              longPressActive   = 0;
byte              savGrnPwm;
uint32_t          interTimer, bounceTimer, buttonTimer, aliveTimer;
volatile uint32_t rotSpinTimer      = 0;
volatile int16_t  newRotVal         = 0;
volatile bool     interruptPending  = false;
volatile bool     sendInterrupt     = false;


//==========================================================================
// look at: https://www.pocketmagic.net/avr-watchdog/
// This function is called upon a HARDWARE RESET:
void wdt_first(void) __attribute__((naked)) __attribute__((section(".init3")));
//==========================================================================

// This is the ISR that is called on each interrupt at _ROTA or _ROTB
// Taken from https://www.best-microcontroller-projects.com/rotary-encoder.html#Taming_Noisy_Rotary_Encoders
//==========================================================================
void updateEncoder()
{
  disablePCINT(digitalPinToPCINT(_ROTB));
  disablePCINT(digitalPinToPCINT(_ROTA));
  
  interruptPending = true;

  prevNextCode <<= 2;
  if (digitalRead(_ROTA)) prevNextCode |= 0x02;
  if (digitalRead(_ROTB)) prevNextCode |= 0x01;
  prevNextCode &= 0x0f;

  // If valid then store as 16 bit data.
  if  (rot_enc_table[prevNextCode] ) {
    store <<= 4;
    store |= prevNextCode;
    if ((store&0xff)==0x2b) {
      newRotVal += (registerStack.rotStep * hwRotDir * modeRotDir);
    }
    if ((store&0xff)==0x17) {
      newRotVal -= (registerStack.rotStep * hwRotDir * modeRotDir);
    }
  }
  rotSpinTimer = millis();

  enablePCINT(digitalPinToPCINT(_ROTA));
  enablePCINT(digitalPinToPCINT(_ROTB));

} // updateEncoder() [ISR]


//==========================================================================
void reBoot()
{
  detachPCINT(digitalPinToPCINT(_ROTB));
  detachPCINT(digitalPinToPCINT(_ROTA));
  cli();  // disable interrupts ..

  analogWrite(_GRNLED,  255);
  analogWrite(_BLUELED, 255);
  for (int b=0; b<10; b++) {
    analogWrite(_REDLED,  0);
    delay(200);
    analogWrite(_REDLED,  255);
    delay(800);
  }

  wdt_enable(WDTO_250MS);
  sei();

  registerStack.status = 0b00000000;

  wdt_reset();
  while (true) {}

} //  reBoot()


//==========================================================================
void handleRotaryEncoder()
{
  //---------------------------------------------------------------
  //-------------- handle rotary encoder status -------------------
  //---------------------------------------------------------------
  if (newRotVal > prevRotaryPos) {
    registerStack.status ^= _BV(COUNTUP_BIT) | _BV(INTERRUPT_BIT);
    if (newRotVal > registerStack.rotMax) {
      newRotVal     = registerStack.rotMax;
      rotSpinTimer  = 0;

      if (registerStack.modeSettings & (1<<STNG_TURNMODE)) {
        if (modeRotDir == 1)
              modeRotDir = -1;
        else  modeRotDir =  1;
      }

      if (registerStack.modeSettings & (1<<STNG_FLIPMODE)) {
        newRotVal   = registerStack.rotMin;
      }
    }

  } else if (newRotVal < prevRotaryPos) {
    registerStack.status ^= _BV(COUNTDOWN_BIT) | _BV(INTERRUPT_BIT);
    if (newRotVal < registerStack.rotMin) {
      newRotVal     = registerStack.rotMin;
      rotSpinTimer  = 0;

      if (registerStack.modeSettings & (1<<STNG_TURNMODE)) {
        if (modeRotDir == 1)  modeRotDir = -1;
        else                  modeRotDir =  1;
      }

      if (registerStack.modeSettings & (1<<STNG_FLIPMODE)) {
        newRotVal   = registerStack.rotMax;
      }
    }
  }

  if (registerStack.status & (1<<INTERRUPT_BIT)) {
    if ((millis() - rotSpinTimer) > registerStack.rotSpinTime) {
      sendInterrupt       = true;
      registerStack.rotVal    = newRotVal;
      prevRotaryPos       = newRotVal;
      rotSpinTimer        = 0;
    }
  }

} // handleRotaryEncoder()


//==========================================================================
void handlePushButton()
{
  //---------------------------------------------------------------
  //-------------- finite state button ----------------------------
  //---------------------------------------------------------------
  switch(btnState) {
  case BTN_INIT:
    if (digitalRead(_BUTTON)) {
      bounceTimer       = micros();
      btnState          = BTN_FIRST_PRESS;
      pressedStateSend  = false;
    }
    break;

  case BTN_FIRST_PRESS:
    if ((micros() - bounceTimer) > registerStack.debounceTime) {
      if (digitalRead(_BUTTON)) {
        btnState    = BTN_IS_PRESSED;
        buttonTimer = millis();
      } else {
        btnState    = BTN_INIT;
      }
    }
    break;

  case BTN_IS_PRESSED:
    if (digitalRead(_BUTTON)) {
      if ((!pressedStateSend) && !(registerStack.status & (1<<PRESSED_BIT))) { // already set?
        registerStack.status ^= _BV(PRESSED_BIT) | _BV(INTERRUPT_BIT);  // button pressed
        sendInterrupt = true;
        pressedStateSend = true;
      }
      if ((millis() - buttonTimer) > 10000) {
        reBoot();
      }
    } else {  // button is released
      bounceTimer = micros();
      btnState    = BTN_FIRST_RELEASE;
    }
    break;

  case BTN_FIRST_RELEASE:
    if ((micros() - bounceTimer) > registerStack.debounceTime) {
      if (!digitalRead(_BUTTON) && (micros() - bounceTimer) > registerStack.debounceTime) {
        btnState    = BTN_IS_RELEASED;
      } else {
        btnState    = BTN_IS_PRESSED;
      }
    }
    break;

  case BTN_IS_RELEASED:
    //aliveTimer = millis();
    if ((millis() - buttonTimer) > registerStack.longPressTime) {
      if (!(registerStack.status & (1<<LONGRELEASE_BIT))) { // already set?
        registerStack.status ^= _BV(LONGRELEASE_BIT) | _BV(INTERRUPT_BIT);  // release Long
        sendInterrupt = true;
        pressedStateSend = false;
      }
    } else if ((millis() - buttonTimer) > registerStack.midPressTime) {
      if (!(registerStack.status & (1<<MIDRELEASE_BIT))) { // already set?
        registerStack.status ^= _BV(MIDRELEASE_BIT) | _BV(INTERRUPT_BIT);  // release Mid
        sendInterrupt = true;
        pressedStateSend = false;
      }
    } else {
      if (!(registerStack.status & (1<<QUICKRELEASE_BIT))) { // already set?
        registerStack.status ^= _BV(QUICKRELEASE_BIT) | _BV(INTERRUPT_BIT);  // release Quick
        sendInterrupt = true;
        pressedStateSend = false;
      }
    }
    btnState    = BTN_INIT;
    bounceTimer = 0;
    break;

  default:
    btnState    = BTN_INIT;

  } // switch()

} // handlePushButton()


//==========================================================================
void setup()
{
  MCUSR=0x00;
  wdt_disable();

  pinMode(_SEND_INT, OUTPUT);
  digitalWrite(_SEND_INT, LOW); // make _SEND_INT LOW
  pinMode(_SEND_INT, INPUT_PULLUP);

  pinMode(_BLUELED,     OUTPUT);
  analogWrite(_BLUELED, _LED_OFF);

  pinMode(_REDLED,     OUTPUT);
  analogWrite(_REDLED, _LED_ON);

  pinMode(_GRNLED,     OUTPUT);
  analogWrite(_GRNLED, _LED_OFF);

  // and enable pullup resisters
  pinMode(_ROTB,     INPUT_PULLUP);
  pinMode(_ROTA,     INPUT_PULLUP);
  pinMode(_BUTTON,   INPUT_PULLUP);

  readConfig();
  startI2C();

  // To make this line work you have to comment out
  // https://github.com/NicoHood/PinChangeInterrupt/blob/master/src/PinChangeInterruptSettings.h#L228
  //
  attachPCINT(digitalPinToPCINT(_ROTA), updateEncoder, CHANGE);
  attachPCINT(digitalPinToPCINT(_ROTB), updateEncoder, CHANGE);

} // setup()


//==========================================================================
void loop()
{
  pinMode(_SEND_INT, INPUT_PULLUP); // Go to high impedance

  if (sendInterrupt) {
    digitalWrite(_SEND_INT, LOW);
    pinMode(_SEND_INT, OUTPUT);
    delay(1);
    //digitalWrite(_SEND_INT, HIGH);
    pinMode(_SEND_INT, INPUT_PULLUP); // Go to high impedance
    delay(25);
    sendInterrupt = false;
    aliveTimer = millis();
  }

  handleRotaryEncoder();
  handlePushButton();

  //---------------------------------------------------------------
  //-------------- now set LED values -----------------------------
  //---------------------------------------------------------------
  analogWrite(_REDLED,  (255 - registerStack.pwmRed));
  analogWrite(_GRNLED,  (255 - registerStack.pwmGreen));
  analogWrite(_BLUELED, (255 - registerStack.pwmBlue));

} // loop()


/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
***************************************************************************/
