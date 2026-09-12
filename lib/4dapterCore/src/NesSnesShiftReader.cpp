/*
 * NesSnesShiftReader.cpp — NES/SNES/PowerPad shift-register reader for 4dapter
 *
 * Copyright (C) 2026 4dapter project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "NesSnesShiftReader.h"

#define NTT_INDICATOR_BIT  0x00
#define SHIFT_BIT_NO_DATA  0x00

// Which of the 32 shifted-in bits are axis (D-pad) bits rather than button
// bits, shared between the NES and SNES decode tables below.
static const uint32_t axisIndicator[32] = {
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint32_t dataMaskNES[8] = {
  0x02,         // A
  0x01,         // B
  0x40,         // Start
  0x80,         // Select
  DPAD_UP,      // D-Up
  DPAD_DOWN,    // D-Down
  DPAD_LEFT,    // D-Left
  DPAD_RIGHT,   // D-Right
};

// Power Pad D4
static const uint32_t dataMaskPowerPadD4[8] = {
  0x08,               // PowerPad #4
  0x04,               // PowerPad #3
  0x800,              // PowerPad #12
  0x80,               // PowerPad #8
  SHIFT_BIT_NO_DATA,  // No Data
  SHIFT_BIT_NO_DATA,  // No Data
  SHIFT_BIT_NO_DATA,  // No Data
  SHIFT_BIT_NO_DATA,  // No Data
};

// Power Pad D3
static const uint32_t dataMaskPowerPadD3[8] = {
  0x02,   // PowerPad #2
  0x01,   // PowerPad #1
  0x10,   // PowerPad #5
  0x100,  // PowerPad #9
  0x20,   // PowerPad #6
  0x200,  // PowerPad #10
  0x400,  // PowerPad #11
  0x40,   // PowerPad #7
};

static const uint32_t dataMaskSNES[32] = {
  0x01,             // B
  0x04,             // Y
  0x40,             // Start
  0x80,             // Select
  DPAD_UP,          // D-Up
  DPAD_DOWN,        // D-Down
  DPAD_LEFT,        // D-Left
  DPAD_RIGHT,       // D-Right
  0x02,             // A
  0x08,             // X
  0x10,             // L
  0x20,             // R
  SHIFT_BIT_NO_DATA, // SNES Control Bit
  NTT_INDICATOR_BIT, // NTT Indicator Bit
  SHIFT_BIT_NO_DATA, // SNES Control Bit
  SHIFT_BIT_NO_DATA, // SNES Control Bit
  0x100,    // NTT 0
  0x200,    // NTT 1
  0x400,    // NTT 2
  0x800,    // NTT 3
  0x1000,   // NTT 4
  0x2000,   // NTT 5
  0x4000,   // NTT 6
  0x8000,   // NTT 7
  0x10000,  // NTT 8
  0x20000,  // NTT 9
  0x40000,  // NTT *
  0x80000,  // NTT #
  0x100000, // NTT .
  0x200000, // NTT C
  SHIFT_BIT_NO_DATA, // NTT No Data
  0x800000, // NTT End Comms
};

NesSnesShiftReader::NesSnesShiftReader()
{
}

void NesSnesShiftReader::init()
{
  // Setup NES / SNES latch and clock pins (2/3 or PD1/PD0)
  DDRD  |=  B00000011; // output
  PORTD &= ~B00000011; // low

  // Setup NES / SNES data pins (A0/A1 or PF6/PF7)
  DDRF  &= ~B11000000; // inputs
  PORTF |=  B11000000; // enable internal pull-ups

  // Setup NES PowerPad data pins (8/9 or PB4/PB5)
  DDRB  &= ~B00110000; // inputs
  PORTB |=  B00110000; // enable internal pull-ups
}

void NesSnesShiftReader::sendLatch()
{
  // Send a latch pulse to NES/SNES
  PORTD |=  B00000010; // Set HIGH
  __builtin_avr_delay_cycles(192);
  PORTD &= ~B00000010; // Set LOW
  __builtin_avr_delay_cycles(72);
}

void NesSnesShiftReader::sendClock()
{
  // Send a clock pulse to NES/SNES
  PORTD |=  B00000001; // Set HIGH
  __builtin_avr_delay_cycles(96);
  PORTD &= ~B00000001; // Set LOW
  __builtin_avr_delay_cycles(72);
}

void NesSnesShiftReader::read(uint32_t& nesButtons, uint32_t& nesAxes, uint32_t& snesButtons, uint32_t& snesAxes)
{
  sendLatch();

  nesButtons = 0;
  nesAxes = 0;
  snesButtons = 0;
  snesAxes = 0;

  bool nttActive = false;

  for (uint8_t dataBitCounter = 0; dataBitCounter < 32; dataBitCounter++)
  {
    // If no NTT controller, end the loop early
    if (!nttActive && dataBitCounter > 13)
    {
      break;
    }

    // NES Power Pad Controller
    if ((dataBitCounter < 8) && ((PINB & B00100000) == 0)) // Power Pad Pin D4 (bottom)
    {
      nesButtons |= dataMaskPowerPadD4[dataBitCounter];
    }

    if ((dataBitCounter < 8) && ((PINB & B00010000) == 0)) // Power Pad Pin D3 (middle)
    {
      nesButtons |= dataMaskPowerPadD3[dataBitCounter];
    }

    // NES Controller
    if ((dataBitCounter < 8) && ((PINF & B10000000) == 0)) // If NES data line is low (indicating a press)
    {
      if (axisIndicator[dataBitCounter])
      {
        nesAxes |= dataMaskNES[dataBitCounter];
      }
      else
      {
        nesButtons |= dataMaskNES[dataBitCounter];
      }
    }

    // SNES / NTT Controller
    if ((PINF & B01000000) == 0) // If SNES data line is low (indicating a press)
    {
      if (dataBitCounter == 13)
      {
        nttActive = true;
      }

      if (axisIndicator[dataBitCounter])
      {
        snesAxes |= dataMaskSNES[dataBitCounter];
      }
      else
      {
        snesButtons |= dataMaskSNES[dataBitCounter];
      }
    }

    sendClock();
  }
}
