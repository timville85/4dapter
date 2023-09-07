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

//Set N64 Joystick Maximum Travel Range (0-127, typically between 75-85 on OEM controllers)
#define N64JoyMax 80

N64Controller       n64_controller;
N64_status_packet   N64Data;

int16_t LeftX = 0;
int16_t LeftY = 0;
int16_t RightX = 0;
int16_t RightY = 0;

#define NES       0
#define SNES      1
#define GENESIS   2

#define BUTTONS   0
#define AXES      1

#define UP        0x01
#define DOWN      0x02
#define LEFT      0x04
#define RIGHT     0x08

#define NTT_BIT   0x00
#define NODATA    0x00

void sendLatch();
void sendClock();
void sendState();

// Manage EEPROM by making sure everything has
// its own index.
enum EEPROMIndices { GENESIS_EEPROM };

// Set up USB HID gamepads
SegaController32U4 controller(GENESIS_EEPROM);

// Controllers
uint32_t  controllerData[2][2] = {{0,0},{0,0}};
uint32_t  axisIndicator[32] = {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint16_t  currentGenesisState = 0;
bool      nttActive = false;

uint32_t  dataMaskNES[8] =        {0x02,   // A
                                   0x01,   // B
                                   0x40,   // Start 
                                   0x80,   // Select
                                   UP,     // D-Up
                                   DOWN,   // D-Down
                                   LEFT,   // D-Left
                                   RIGHT   // D-Right
                                   }; 

// Power Pad D4
uint32_t  dataMaskPowerPadD4[8] = {0x08,    // PowerPad #4
                                   0x04,    // PowerPad #3
                                   0x800,   // PowerPad #12
                                   0x80,    // PowerPad #8
                                   NODATA,  // No Data
                                   NODATA,  // No Data
                                   NODATA,  // No Data
                                   NODATA   // No Data
                                   }; 

// Power Pad D3
uint32_t  dataMaskPowerPadD3[8] = {0x02,   // PowerPad #2
                                   0x01,   // PowerPad #1
                                   0x10,   // PowerPad #5
                                   0x100,  // PowerPad #9
                                   0x20,   // PowerPad #6
                                   0x200,  // PowerPad #10
                                   0x400,  // PowerPad #11
                                   0x40    // PowerPad #7
                                   }; 

uint32_t  dataMaskSNES[32] =      {0x01,    // B
                                   0x04,    // Y
                                   0x40,    // Start   
                                   0x80,    // Select
                                   UP,      // D-Up
                                   DOWN,    // D-Down
                                   LEFT,    // D-Left
                                   RIGHT,   // D-Right
                                   0x02,    // A
                                   0x08,    // X
                                   0x10,    // L
                                   0x20,    // R
                                   NODATA,  // SNES Control Bit
                                   NTT_BIT, // NTT Indicator Bit
                                   NODATA,  // SNES Control Bit
                                   NODATA,  // SNES Control Bit
                                   0x100,   // NTT 0
                                   0x200,   // NTT 1
                                   0x400,   // NTT 2
                                   0x800,   // NTT 3
                                   0x1000,  // NTT 4
                                   0x2000,  // NTT 5
                                   0x4000,  // NTT 6
                                   0x8000,  // NTT 7
                                   0x10000, // NTT 8
                                   0x20000, // NTT 9
                                   0x40000, // NTT *
                                   0x80000, // NTT #
                                   0x100000,// NTT .
                                   0x200000,// NTT C
                                   NODATA,  // NTT No Data
                                   0x800000,// NTT End Comms
                                   };

void setup()
{
  XInput.setAutoSend(false);
  XInput.setRange(JOY_LEFT,  0, 255);
  XInput.setRange(JOY_RIGHT, 0, 255);
  XInput.setRange(TRIGGER_RIGHT, 0, 1);

  n64_controller.N64_init();

  // N64 Data pin setup
  DDRD  &= ~B00010000; // inputs
  PORTD |=  B00010000; // enable internal pull-ups
  
  // Setup NES / SNES latch and clock pins (2/3 or PD1/PD0)
  DDRD  |=  B00000011; // output
  PORTD &= ~B00000011; // low

  // Setup NES / SNES data pins (A0/A1 or PF6/PF7)
  DDRF  &= ~B11000000; // inputs
  PORTF |=  B11000000; // enable internal pull-ups

  // Setup NES PowerPad data pins (8/9 or PB4/PB5)
  DDRB  &= ~B00110000; // inputs
  PORTB |=  B00110000; // enable internal pull-ups

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
    
    for(uint8_t j = 0; j < 1; j++)
    {
      sendLatch();

      controllerData[NES][BUTTONS] = 0;
      controllerData[NES][AXES] = 0;
      
      controllerData[SNES][BUTTONS] = 0;
      controllerData[SNES][AXES] = 0;

      nttActive = false;
  
      for(uint8_t dataBitCounter = 0; dataBitCounter < 32; dataBitCounter++)
      {
        // If no NTT controller, end the loop early
        if(!nttActive && dataBitCounter > 13)
        {
          break;
        }

        //NES Power Pad Controller
        if((dataBitCounter < 8) && ((PINB & B00100000) == 0)) //Power Pad Pin D4 (bottom)
        { 
          controllerData[NES][BUTTONS] |= dataMaskPowerPadD4[dataBitCounter];
        }

        if((dataBitCounter < 8) && ((PINB & B00010000) == 0)) //Power Pad Pin D3 (middle)
        { 
          controllerData[NES][BUTTONS] |= dataMaskPowerPadD3[dataBitCounter];
        }

        // NES Controller
        if((dataBitCounter < 8) && ((PINF & B10000000) == 0)) //If NES data line is low (indicating a press)
        { 
          if(axisIndicator[dataBitCounter])
          {
            controllerData[NES][AXES] |= dataMaskNES[dataBitCounter];
          }
          else
          {
            controllerData[NES][BUTTONS] |= dataMaskNES[dataBitCounter];
          }
        }

        // SNES / NTT Controller 
        if((PINF & B01000000) == 0) //If SNES data line is low (indicating a press)
        {
          if(dataBitCounter == 13)
          {
            nttActive = true;
          }
          
          if(axisIndicator[dataBitCounter])
          {
            controllerData[SNES][AXES] |= dataMaskSNES[dataBitCounter];
          }
          else
          {
            controllerData[SNES][BUTTONS] |= dataMaskSNES[dataBitCounter];
          }
        }
        
        sendClock();
      }
    }    

  n64_controller.getN64Packet();
  N64Data = n64_controller.N64_status;
  
  sendState();
}

void sendLatch()
{
  // Send a latch pulse to NES/SNES
  PORTD |=  B00000010; // Set HIGH
  __builtin_avr_delay_cycles(192);
  PORTD &= ~B00000010; // Set LOW
  __builtin_avr_delay_cycles(72);
}

void sendClock()
{
  // Send a clock pulse to NES/SNES
  PORTD |=  B00000001; // Set HIGH
  __builtin_avr_delay_cycles(96);
  PORTD &= ~B00000001; // Set LOW
  __builtin_avr_delay_cycles(72);
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
  XInput.setButton(BUTTON_A,   (controllerData[NES][BUTTONS] & 0x01)  | (controllerData[SNES][BUTTONS] & 0x01) | (currentGenesisState & SC_BTN_B)     | (N64Data.data1 & 0x80 ? 1:0) );
  XInput.setButton(BUTTON_B,   (controllerData[NES][BUTTONS] & 0x02)  | (controllerData[SNES][BUTTONS] & 0x02) | (currentGenesisState & SC_BTN_C)     | (N64Data.data2 & 0x04 ? 1:0) );
  XInput.setButton(BUTTON_X,   (controllerData[SNES][BUTTONS] & 0x04) | (currentGenesisState & SC_BTN_A) | (N64Data.data1 & 0x40 ? 1:0));
  XInput.setButton(BUTTON_Y,   (controllerData[SNES][BUTTONS] & 0x08) | (currentGenesisState & SC_BTN_Y) | (N64Data.data2 & 0x02 ? 1:0));
  XInput.setTrigger(TRIGGER_RIGHT, (N64Data.data1 & 0x20 ? 1:0));

  XInput.setDpad( (controllerData[NES][AXES] & UP)          |  (controllerData[SNES][AXES] & UP)          | ((currentGenesisState & SC_BTN_UP) >> SC_BIT_SH_UP)       | (N64Data.data1 & 0x08 ? 1:0), 
                 ((controllerData[NES][AXES] & DOWN) >> 1)  | ((controllerData[SNES][AXES] & DOWN) >> 1)  | ((currentGenesisState & SC_BTN_DOWN) >> SC_BIT_SH_DOWN)   | (N64Data.data1 & 0x04 ? 1:0), 
                 ((controllerData[NES][AXES] & LEFT) >> 2)  | ((controllerData[SNES][AXES] & LEFT) >> 2)  | ((currentGenesisState & SC_BTN_LEFT) >> SC_BIT_SH_LEFT)   | (N64Data.data1 & 0x02 ? 1:0), 
                 ((controllerData[NES][AXES] & RIGHT) >> 3) | ((controllerData[SNES][AXES] & RIGHT) >> 3) | ((currentGenesisState & SC_BTN_RIGHT) >> SC_BIT_SH_RIGHT) | (N64Data.data1 & 0x01 ? 1:0),
                true);

  XInput.setButton(BUTTON_LB,     (controllerData[SNES][BUTTONS] & 0x10) | (currentGenesisState & SC_BTN_X) | (N64Data.data2 & 0x20 ? 1:0));
  XInput.setButton(BUTTON_RB,     (controllerData[SNES][BUTTONS] & 0x20) | (currentGenesisState & SC_BTN_Z) | (N64Data.data2 & 0x10 ? 1:0));
  XInput.setButton(BUTTON_BACK,   (controllerData[NES][BUTTONS] & 0x40) | (controllerData[SNES][BUTTONS] & 0x40) | (currentGenesisState & SC_BTN_MODE)  | (N64Data.data2 & 0x08 ? 1:0) );
  XInput.setButton(BUTTON_START,  (controllerData[NES][BUTTONS] & 0x80) | (controllerData[SNES][BUTTONS] & 0x80) | (currentGenesisState & SC_BTN_START) | (N64Data.data1 & 0x10 ? 1:0) );
  XInput.setButton(BUTTON_LOGO,   (currentGenesisState & SC_BTN_HOME));

  /*
  if     ((N64Data.data2 & 0x08 ? 1:0)) RightY = 255;   // C-Up
  else if((N64Data.data2 & 0x04 ? 1:0)) RightY = 0;     // C-Down
  else                                  RightY = 128;   // Neither C-Up / C-Down

  if     ((N64Data.data2 & 0x02 ? 1:0)) RightX = 0;     // C Left
  else if((N64Data.data2 & 0x01 ? 1:0)) RightX = 255;   // C Right
  else                                  RightX = 128;   // Neither C-Left / C-Right
  */

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
