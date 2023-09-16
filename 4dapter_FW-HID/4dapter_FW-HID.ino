
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

#include "SegaController32U4.h"
#include "Gamepad.h"
#include "N64_Controller.h"

// ATT: 20 chars max (including NULL at the end) according to Arduino source code.
// Additionally serial number is used to differentiate arduino projects to have different button maps!
const char *gp_serial = "4DAPTER";

#define N64MapJoyToMax  true  // 'true' to map value to DInput Max (-128 to +127), set to false to use controller value directly
#define N64JoyMax       80     // N64 Joystick Maximum Travel Range (0-127, typically between 75-85 on OEM controllers)
#define N64JoyDeadzone  3      // Deadzone to return 0, minimizes drift

N64Controller       n64_controller;
N64_status_packet   N64Data;
int8_t LeftX = 0;
int8_t LeftY = 0;

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

// Controller DB9 pins (looking face-on to the end of the plug):
// 5 4 3 2 1
//  9 8 7 6
//
// Wire it all up according to the following table:
//
// Triple Controller    V1            V2        *** = V1 to V2 Change
// ------------------------------------------------------------------
// VCC                  VCC ()        VCC ()
// GND                  GND ()        GND ()
// Shared-LATCH         2   (PD1)     2   (PD1)
// Shared-CLOCK         3   (PD0)     3   (PD0)
// NES-Data1 (4)        A0  (PF7)     A0  (PF7)
// NES-DataD4 (5)       N/C           9   (PB5) ***
// NES-DataD3 (6)       N/C           8   (PB4) ***
// SNES-Data1 (4)       A1  (PF6)     A1  (PF6)
// SNES-DataD2 (5)      N/C           RX  (PD2) ***
// SNES-DataD3 (6)      N/C           TX  (PD3) ***
// DB9-1                5   (PC6)     5   (PC6)
// DB9-2                6   (PD7)     6   (PB2)
// DB9-3                A2  (PF5)     A2  (PF5)
// DB9-4                A3  (PF4)     A3  (PF4)
// DB9-5                VCC ()        16  (PB2) ***
// DB9-6                14  (PB3)     14  (PB3)
// DB9-7                7   (PE6)     7   (PE6)
// DB9-8                GND ()        GND ()
// DB9-9                15  (PB1)     15  (PB1)

/* Power Pad Number Guide
 * C/O: http://www.neshq.com/hardgen/powerpad.txt

+---------/          \-----------+
|                       SIDE B   |
|   __      __       __     __   |
|  /  \    /  \     /  \   /  \  |
| | 1  |  | 2  |   | 3  | | 4  | |
|  \__/    \__/     \__/   \__/  |
|   __      __       __     __   |
|  /  \    /  \     /  \   /  \  |
| | 5  |  | 6  |   | 7  | | 8  | |
|  \__/    \__/     \__/   \__/  |
|   __      __       __     __   |
|  /  \    /  \     /  \   /  \  |
| | 9  |  | 10 |   | 11 | | 12 | |
|  \__/    \__/     \__/   \__/  |
|                                |
+--------------------------------+

           __________
+---------/          \-----------+
|                       SIDE A   |
|           __       __          |
|          /  \     /  \         |
|         |B 3 |   |B 2 |        |
|          \__/     \__/         |
|   __      __       __     __   |
|  /  \    /  \     /  \   /  \  |
| |B 8 |  |R 7 |   |R 6 | |B 5 | |
|  \__/    \__/     \__/   \__/  |
|           __       __          |
|          /  \     /  \         |
|         |B 11|   |B 10|        |
|          \__/     \__/         |
|                                |
+--------------------------------+
*/

// Manage EEPROM by making sure everything has
// its own index.
enum EEPROMIndices { GENESIS_EEPROM };

// Set up USB HID gamepads
Gamepad_ Gamepad[3];

SegaController32U4 controller(GENESIS_EEPROM);

// Controllers
uint32_t  controllerData[2][2] = {{0,0},{0,0}};
uint32_t  axisIndicator[32] = {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint16_t  currentState = 0;
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
  while(true)
  {
    //8 cycles needed to capture 6-button controllers
    for(uint8_t i = 0; i < 8; i++)
    {
      currentState = controller.updateState();
    }

    currentState = controller.getFinalState();
    Gamepad[1]._GamepadReport.buttons = currentState >> 4;

    if      (((currentState & SC_BTN_DOWN) >> SC_BIT_SH_DOWN))    Gamepad[1]._GamepadReport.Y = 0x7F;
    else if (((currentState & SC_BTN_UP) >> SC_BIT_SH_UP))        Gamepad[1]._GamepadReport.Y = 0x80;
    else                                                          Gamepad[1]._GamepadReport.Y = 0;

    if      (((currentState & SC_BTN_RIGHT) >> SC_BIT_SH_RIGHT))  Gamepad[1]._GamepadReport.X = 0x7F;
    else if (((currentState & SC_BTN_LEFT) >> SC_BIT_SH_LEFT))    Gamepad[1]._GamepadReport.X = 0x80;
    else                                                          Gamepad[1]._GamepadReport.X = 0;

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
  
      Gamepad[0]._GamepadReport.buttons = controllerData[NES][BUTTONS] | controllerData[SNES][BUTTONS];
      
      if      ( ((controllerData[NES][AXES] & DOWN) >> 1) | ((controllerData[SNES][AXES] & DOWN) >> 1))  Gamepad[0]._GamepadReport.Y = 0x7F;
      else if (  (controllerData[NES][AXES] & UP  )       |  (controllerData[SNES][AXES] & UP  )      )  Gamepad[0]._GamepadReport.Y = 0x80;
      else    Gamepad[0]._GamepadReport.Y = 0;

      if      ( ((controllerData[NES][AXES] & RIGHT) >> 3) | ((controllerData[SNES][AXES] & RIGHT) >> 3))  Gamepad[0]._GamepadReport.X = 0x7F;
      else if ( ((controllerData[NES][AXES] & LEFT ) >> 2) | ((controllerData[SNES][AXES] & LEFT ) >> 2))  Gamepad[0]._GamepadReport.X = 0x80;
      else    Gamepad[0]._GamepadReport.X = 0;
      
      n64_controller.getN64Packet();
      N64Data = n64_controller.N64_status;
      
      if(N64Data.stick_x >= -N64JoyDeadzone && N64Data.stick_x <= N64JoyDeadzone)
      {
        LeftX = 0;
      }
      else
      {
        if(N64MapJoyToMax)
        {
          LeftX = map(N64Data.stick_x, -N64JoyMax, N64JoyMax, -128, 127);
          if(N64Data.stick_x > N64JoyMax)   N64Data.stick_x = N64JoyMax;
          if(N64Data.stick_x < -N64JoyMax)  N64Data.stick_x = -N64JoyMax;
        }
        else
        {
          LeftX = (int8_t)N64Data.stick_x;
        }
      }

      if(N64Data.stick_y >= -N64JoyDeadzone && N64Data.stick_y <= N64JoyDeadzone)
      {
        LeftY = 0;
      }
      else
      {
        if(N64MapJoyToMax)
        {
          if(N64Data.stick_y > N64JoyMax)   N64Data.stick_y = N64JoyMax;
          if(N64Data.stick_y < -N64JoyMax)  N64Data.stick_y = -N64JoyMax;
          LeftY = map(-N64Data.stick_y, -N64JoyMax, N64JoyMax, -128, 127); 
        }
        else
        {
          LeftY = (int8_t) -N64Data.stick_y;
        }
      }

      Gamepad[2]._GamepadReport.buttons = 0;
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x80 ? 1:0) << 1;  // A 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x40 ? 1:0) << 0;  // B  

      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x10 ? 1:0) << 7;  // Start 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x08 ? 1:0) << 9;  // Dup 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x04 ? 1:0) << 10; // Ddown 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x02 ? 1:0) << 11; // Dleft 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x01 ? 1:0) << 12; // Dright

      if( (N64Data.data2 & 0x20 ? 1:0) && (N64Data.data2 & 0x10 ? 1:0) && (N64Data.data2 & 0x04 ? 1:0) )
      {
        Gamepad[2]._GamepadReport.buttons |= 1 << 6; // Select
        LeftY = 127;
      }
      else
      {
        Gamepad[2]._GamepadReport.buttons |= (N64Data.data2 & 0x20 ? 1:0) << 4;  // L 
        Gamepad[2]._GamepadReport.buttons |= (N64Data.data2 & 0x10 ? 1:0) << 5;  // R
        Gamepad[2]._GamepadReport.buttons |= (N64Data.data1 & 0x20 ? 1:0) << 8;  // Z  
      }

      Gamepad[2]._GamepadReport.buttons |= (N64Data.data2 & 0x08 ? 1:0) << 13; // Cup
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data2 & 0x04 ? 1:0) << 3;  // Cdown 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data2 & 0x02 ? 1:0) << 2;  // Cleft 
      Gamepad[2]._GamepadReport.buttons |= (N64Data.data2 & 0x01 ? 1:0) << 14; // Cright

      Gamepad[2]._GamepadReport.buttons &= 0x0000FFFF;
      
      Gamepad[2]._GamepadReport.X = LeftX;
      Gamepad[2]._GamepadReport.Y = LeftY;
    }    

  sendState();
 }
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

void sendState()
{
  Gamepad[0].send();
  Gamepad[1].send();
  Gamepad[2].send();
  __builtin_avr_delay_cycles(16000);
}
