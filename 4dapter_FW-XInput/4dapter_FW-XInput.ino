/*
 *  GNU GENERAL PUBLIC LICENSE
 *  Version 3, 29 June 2007
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <XInput.h>
#include "SegaController32U4.h"
#include "N64_Controller.h"
#include "NesSnesShiftReader.h"

//Set N64 Joystick Maximum Travel Range (0-127, typically between 75-85 on OEM controllers)
#define N64JoyMax 80

N64Controller       n64_controller;
N64_status_packet   N64Data;

int16_t LeftX = 128;
int16_t LeftY = 128;
int16_t RightX = 128;
int16_t RightY = 128;

void sendState();

// Manage EEPROM by making sure everything has
// its own index.
enum EEPROMIndices { GENESIS_EEPROM };

SegaController32U4 controller(GENESIS_EEPROM);
NesSnesShiftReader nesSnesReader;

// NES / SNES shift-register read results for the current cycle.
uint32_t  nesButtons = 0;
uint32_t  nesAxes = 0;
uint32_t  snesButtons = 0;
uint32_t  snesAxes = 0;

uint16_t  currentGenesisState = 0;

void setup()
{
  XInput.setAutoSend(false);
  XInput.setRange(JOY_LEFT,  0, 255);
  XInput.setRange(JOY_RIGHT, 0, 255);
  XInput.setRange(TRIGGER_RIGHT, 0, 1);

  n64_controller.N64_init();
  nesSnesReader.init();

  // N64 Data pin setup
  DDRD  &= ~B00010000; // inputs
  PORTD |=  B00010000; // enable internal pull-ups

  // Setup power pin (DB9 Pin 5) as output high (PB2)
  DDRB  |= B00000100; // output
  PORTB |= B00000100; // high

  delay(250);
}

void loop()
{
    currentGenesisState = 0;

    //8 cycles needed to capture 6-button controllers
    for(uint8_t i = 0; i < 8; i++)
    {
      currentGenesisState = controller.updateState();
    }

    currentGenesisState = controller.getFinalState();

    nesSnesReader.read(nesButtons, nesAxes, snesButtons, snesAxes);

  n64_controller.getN64Packet();
  N64Data = n64_controller.N64_status;

  sendState();
}

/*
  A -      (N64Data.data1 & 0x80 ? 1:0)
  B -      (N64Data.data1 & 0x40 ? 1:0)
  Z -      (N64Data.data1 & 0x20 ? 1:0)
  Start -  (N64Data.data1 & 0x10 ? 1:0)
  Dup -    (N64Data.data1 & 0x08 ? 1:0)
  Ddown -  (N64Data.data1 & 0x04 ? 1:0)
  Dleft -  (N64Data.data1 & 0x02 ? 1:0)
  Dright - (N64Data.data1 & 0x01 ? 1:0)

  Reset -  (N64Data.data2 & 0x80 ? 1:0)
  L -      (N64Data.data2 & 0x20 ? 1:0)
  R -      (N64Data.data2 & 0x10 ? 1:0)
  Cup -    (N64Data.data2 & 0x08 ? 1:0)
  Cdown -  (N64Data.data2 & 0x04 ? 1:0)
  Cleft -  (N64Data.data2 & 0x02 ? 1:0)
  Cright - (N64Data.data2 & 0x01 ? 1:0)

  X-Axis - N64Data.stick_x
  Y-Axis - N64Data.stick_y

 */

void sendState()
{
  // Reset all cached button presses to force a full update every cycle.
  XInput.releaseAll();

  // Use controller data from all 3 inputs to map to single XInput Controller (takes about 350us per controller cycle)
  XInput.setButton(BUTTON_A,   (nesButtons & 0x01)  | (snesButtons & 0x01) | (currentGenesisState & SC_BTN_B)     | (N64Data.data1 & 0x80 ? 1:0) );
  XInput.setButton(BUTTON_B,   (nesButtons & 0x02)  | (snesButtons & 0x02) | (currentGenesisState & SC_BTN_C)     | (N64Data.data2 & 0x04 ? 1:0) );
  XInput.setButton(BUTTON_X,   (snesButtons & 0x04) | (currentGenesisState & SC_BTN_A) | (N64Data.data1 & 0x40 ? 1:0));
  XInput.setButton(BUTTON_Y,   (snesButtons & 0x08) | (currentGenesisState & SC_BTN_Y) | (N64Data.data2 & 0x02 ? 1:0));
  XInput.setButton(BUTTON_L3,  (N64Data.data2 & 0x08 ? 1:0));
  XInput.setButton(BUTTON_R3,  (N64Data.data2 & 0x01 ? 1:0));
  XInput.setTrigger(TRIGGER_RIGHT, (N64Data.data1 & 0x20 ? 1:0));

  XInput.setDpad( (nesAxes & DPAD_UP)          |  (snesAxes & DPAD_UP)          | ((currentGenesisState & SC_BTN_UP) >> SC_BIT_SH_UP)       | (N64Data.data1 & 0x08 ? 1:0),
                 ((nesAxes & DPAD_DOWN) >> 1)  | ((snesAxes & DPAD_DOWN) >> 1)  | ((currentGenesisState & SC_BTN_DOWN) >> SC_BIT_SH_DOWN)   | (N64Data.data1 & 0x04 ? 1:0),
                 ((nesAxes & DPAD_LEFT) >> 2)  | ((snesAxes & DPAD_LEFT) >> 2)  | ((currentGenesisState & SC_BTN_LEFT) >> SC_BIT_SH_LEFT)   | (N64Data.data1 & 0x02 ? 1:0),
                 ((nesAxes & DPAD_RIGHT) >> 3) | ((snesAxes & DPAD_RIGHT) >> 3) | ((currentGenesisState & SC_BTN_RIGHT) >> SC_BIT_SH_RIGHT) | (N64Data.data1 & 0x01 ? 1:0),
                true);

  XInput.setButton(BUTTON_LB,     (snesButtons & 0x10) | (currentGenesisState & SC_BTN_X) | (N64Data.data2 & 0x20 ? 1:0));
  XInput.setButton(BUTTON_RB,     (snesButtons & 0x20) | (currentGenesisState & SC_BTN_Z) | (N64Data.data2 & 0x10 ? 1:0));
  XInput.setButton(BUTTON_BACK,   (nesButtons & 0x40) | (snesButtons & 0x40) | (currentGenesisState & SC_BTN_MODE) );
  XInput.setButton(BUTTON_START,  (nesButtons & 0x80) | (snesButtons & 0x80) | (currentGenesisState & SC_BTN_START) | (N64Data.data1 & 0x10 ? 1:0) );
  XInput.setButton(BUTTON_LOGO,   (currentGenesisState & SC_BTN_HOME));


  if(N64Data.data2 & 0x80) //Use N64 "reset" as Back button without sending shoulder buttons
  {
    XInput.setButton(BUTTON_LB,   0);
    XInput.setButton(BUTTON_RB,   0);
    XInput.setButton(BUTTON_BACK, 1);
  }

  //////////////////////////////////////////

  N64Data.stick_y = -N64Data.stick_y;

  if(N64Data.stick_x > N64JoyMax)   N64Data.stick_x = N64JoyMax;
  if(N64Data.stick_x < -N64JoyMax)  N64Data.stick_x = -N64JoyMax;
  if(N64Data.stick_y > N64JoyMax)   N64Data.stick_y = N64JoyMax;
  if(N64Data.stick_y < -N64JoyMax)  N64Data.stick_y = -N64JoyMax;

  LeftX = map(N64Data.stick_x, -N64JoyMax, N64JoyMax, 0, 255);
  LeftY = map(-N64Data.stick_y, -N64JoyMax, N64JoyMax, 0, 255);

  XInput.setJoystick(JOY_LEFT, LeftX, LeftY);
  XInput.setJoystick(JOY_RIGHT, RightX, RightY);

  // Takes about 1.4-1.5ms to send USB Packet on MiSTer (at 1000hz polling rate)
  // Takes about 3-4ms to send USB Packet on Analogue Pocket Dock (likely 250hz polling rate)
  XInput.send();
}
