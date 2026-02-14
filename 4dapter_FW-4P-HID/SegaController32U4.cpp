/*
 * SegaController32U4.cpp — Genesis/Mega Drive controller (DB9) for 4dapter
 *
 * Copyright (C) 2026 4dapter project
 *
 * Original Genesis/MD controller code by Jon Thysell (2017); interfacing
 * largely rewritten by Mikael Norrgård. This version modified for 4dapter.
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

#include "Arduino.h"
#include "SegaController32U4.h"

SegaController32U4::SegaController32U4()
{
  // Setup select pin as output high (7, PE6)
  DDR_SELECT |= MASK_SELECT; // output
  PORT_SELECT |= MASK_SELECT; // high

  // Setup input pins (A0,A1,A2,A3,14,15 or PF7,PF6,PF5,PF4,PB3,PB1)
  DDRF &= ~B00110000; // input
  PORTF |= B00110000; // high to enable internal pull-up
  DDRB &= ~B00001010; // input
  PORTB |= B00001010; // high to enable internal pull-up
  DDRC &= ~B01000000; // input
  PORTC |= B01000000; // high to enable internal pull-up
  DDRD &= ~B10000000; // input
  PORTD |= B10000000; // high to enable internal pull-up

  _inputReg1 = 0;
  _inputReg2 = 0;
  _inputReg3 = 0;
  _inputReg4 = 0;
  _currentState = 0;
  _connected = 0;
  _sixButtonMode = false;
  _ignoreCycles = 0;
  _pinSelect = true;
}

word SegaController32U4::updateState()
{
  // "Normal" Six button controller reading routine, done a bit differently in this project
  // Cycle  TH out  TR in  TL in  D3 in  D2 in  D1 in  D0 in
  // 0      LO      Start  A      0      0      Down   Up
  // 1      HI      C      B      Right  Left   Down   Up
  // 2      LO      Start  A      0      0      Down   Up      (Check connected and read Start and A in this cycle)
  // 3      HI      C      B      Right  Left   Down   Up      (Read B, C and directions in this cycle)
  // 4      LO      Start  A      0      0      0      0       (Check for six button controller in this cycle)
  // 5      HI      C      B      Mode   X      Y      Z       (Read X,Y,Z and Mode in this cycle)
  // 6      LO      ---    ---    ---    ---    ---    Home    (Home only for 8bitdo wireless gamepads)
  // 7      HI      ---    ---    ---    ---    ---    ---

  // Set the select pin low/high
  _pinSelect = !_pinSelect;
  (!_pinSelect) ? PORT_SELECT &= ~MASK_SELECT : PORT_SELECT |= MASK_SELECT; // Set LOW on even cycle, HIGH on uneven cycle

  // Short delay to stabilise outputs in controller
  delayMicroseconds(SC_CYCLE_DELAY);

  // Read input register(s)
  _inputReg1 = PINF;
  _inputReg2 = PINB;
  _inputReg3 = PINC;
  _inputReg4 = PIND;

  if(_ignoreCycles <= 0)
  {
    if(_pinSelect) // Select pin is HIGH
    {
      if(_connected)
      {
        // Check if six button mode is active
        if(_sixButtonMode)
        {
          // Read input pins for X, Y, Z, Mode
          (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW) ? _currentState |= SC_BTN_Z : _currentState &= ~SC_BTN_Z;
          (bitRead(_inputReg4, DB9_PIN2_BIT) == LOW) ? _currentState |= SC_BTN_Y : _currentState &= ~SC_BTN_Y;
          (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW) ? _currentState |= SC_BTN_X : _currentState &= ~SC_BTN_X;
          (bitRead(_inputReg1, DB9_PIN4_BIT) == LOW) ? _currentState |= SC_BTN_MODE : _currentState &= ~SC_BTN_MODE;
          _sixButtonMode = false;
          _ignoreCycles = 2; // Ignore the two next cycles (cycles 6 and 7 in table above)
        }
        else
        {
          // Read input pins for Up, Down, Left, Right, B, C
          (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW) ? _currentState |= SC_BTN_UP : _currentState &= ~SC_BTN_UP;
          (bitRead(_inputReg4, DB9_PIN2_BIT) == LOW) ? _currentState |= SC_BTN_DOWN : _currentState &= ~SC_BTN_DOWN;
          (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW) ? _currentState |= SC_BTN_LEFT : _currentState &= ~SC_BTN_LEFT;
          (bitRead(_inputReg1, DB9_PIN4_BIT) == LOW) ? _currentState |= SC_BTN_RIGHT : _currentState &= ~SC_BTN_RIGHT;
          (bitRead(_inputReg2, DB9_PIN6_BIT) == LOW) ? _currentState |= SC_BTN_B : _currentState &= ~SC_BTN_B;
          (bitRead(_inputReg2, DB9_PIN9_BIT) == LOW) ? _currentState |= SC_BTN_C : _currentState &= ~SC_BTN_C;
        }
      }
      else // No Mega Drive controller is connected, use SMS/Atari mode
      {
        // Clear current state
        _currentState = 0;

        // Read input pins for Up, Down, Left, Right, Fire1, Fire2
        if (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW)
        {
          _currentState |= SC_BTN_UP;
        }
        if (bitRead(_inputReg4, DB9_PIN2_BIT) == LOW)
        {
          _currentState |= SC_BTN_DOWN;
        }
        if (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW)
        {
          _currentState |= SC_BTN_LEFT;
        }
        if (bitRead(_inputReg1, DB9_PIN4_BIT) == LOW)
        {
          _currentState |= SC_BTN_RIGHT;
        }
        if (bitRead(_inputReg2, DB9_PIN6_BIT) == LOW)
        {
          _currentState |= SC_BTN_A;
        }
        if (bitRead(_inputReg2, DB9_PIN9_BIT) == LOW)
        {
          _currentState |= SC_BTN_B;
        }
      }
    }
    else // Select pin is LOW
    {
      // Check if a controller is connected
      _connected = (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW && bitRead(_inputReg1, DB9_PIN4_BIT) == LOW);

      // Check for six button mode
      _sixButtonMode = (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW && bitRead(_inputReg4, DB9_PIN2_BIT) == LOW);

      // Read input pins for A and Start
      if(_connected)
      {
        if(!_sixButtonMode)
        {
          (bitRead(_inputReg2, DB9_PIN6_BIT) == LOW) ? _currentState |= SC_BTN_A : _currentState &= ~SC_BTN_A;
          (bitRead(_inputReg2, DB9_PIN9_BIT) == LOW) ? _currentState |= SC_BTN_START : _currentState &= ~SC_BTN_START;
        }
      }
    }
  }
  else
  {
    if(_ignoreCycles-- == 2) // Decrease the ignore cycles counter and read 8bitdo home in first "ignored" cycle, this cycle is unused on normal 6-button controllers
    {
      (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW) ? _currentState |= SC_BTN_HOME : _currentState &= ~SC_BTN_HOME;
    }
  }

  return _currentState;
}

word SegaController32U4::getFinalState()
{
  return _currentState;
}
