//
// SegaController32U4.cpp
//
// Authors:
//       Jon Thysell <thysell@gmail.com>
//       Mikael Norrgård <mick@daemonbite.com>
//
// (Based on the code by Jon Thysell, but the interfacing is almost completely
//  rewritten by Mikael Norrgård)
//
// Copyright (c) 2017 Jon Thysell <http://jonthysell.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <EEPROM.h>

#include "Arduino.h"
#include "SegaController32U4.h"

// Value that triggers MiSTer mode if in EEPROM
static const char kMisterModeChar = 'M';

SegaController32U4::SegaController32U4(int eeprom_index)
    : _eeprom_index(eeprom_index)
{
    // Setup select pin as output high (7, PE6)
    DDR_SELECT  |= MASK_SELECT; // output
    PORT_SELECT |= MASK_SELECT; // high

    // Setup input pins (A0,A1,A2,A3,14,15 or PF7,PF6,PF5,PF4,PB3,PB1)
    DDRF  &= ~B00110000; // input
    PORTF |=  B00110000; // high to enable internal pull-up
    DDRB  &= ~B00001010; // input
    PORTB |=  B00001010; // high to enable internal pull-up
    DDRC  &= ~B01000000; // input
    PORTC |=  B01000000; // high to enable internal pull-up
    DDRD  &= ~B10000000; // input
    PORTD |=  B10000000; // high to enable internal pull-up   
    
    _inputReg1 = 0;
    _inputReg2 = 0;
    _inputReg3 = 0;
    _inputReg4 = 0;
    _currentState = 0;
    _previousState = 0;
    _misterMode = (EEPROM.read(_eeprom_index) == kMisterModeChar);
    _connected = 0;
    sixButtonMode = false;
    _ignoreCycles = 0;
    _pinSelect = true;
}

void SegaController32U4::toggleMisterMode() {
  _misterMode = !_misterMode;
  const char value = _misterMode ? kMisterModeChar : 0;
  EEPROM.write(_eeprom_index, value);
}

bool SegaController32U4::isMisterMode() {
    return _misterMode;
}

// Takes a state and swap btn_1 and btn_2
static void doSwapbuttons(word *state, int btn_1, int btn_2) {
  const bool one_is_set = (*state) & btn_1;
  const bool two_is_set = (*state) & btn_2;
  // Flip bit polarity when the two buttons have different states
  if (one_is_set ^ two_is_set) {
    *state ^= btn_1 | btn_2;
  }
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
        if(sixButtonMode)
        {
          // Read input pins for X, Y, Z, Mode
          (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW) ? _currentState |= SC_BTN_Z : _currentState &= ~SC_BTN_Z;
          (bitRead(_inputReg4, DB9_PIN2_BIT) == LOW) ? _currentState |= SC_BTN_Y : _currentState &= ~SC_BTN_Y;
          (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW) ? _currentState |= SC_BTN_X : _currentState &= ~SC_BTN_X;
          (bitRead(_inputReg1, DB9_PIN4_BIT) == LOW) ? _currentState |= SC_BTN_MODE : _currentState &= ~SC_BTN_MODE;
          sixButtonMode = false;
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
        if (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW) { _currentState |= SC_BTN_UP; }
        if (bitRead(_inputReg4, DB9_PIN2_BIT) == LOW) { _currentState |= SC_BTN_DOWN; }
        if (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW) { _currentState |= SC_BTN_LEFT; }
        if (bitRead(_inputReg1, DB9_PIN4_BIT) == LOW) { _currentState |= SC_BTN_RIGHT; }
        if (bitRead(_inputReg2, DB9_PIN6_BIT) == LOW) { _currentState |= SC_BTN_A; }
        if (bitRead(_inputReg2, DB9_PIN9_BIT) == LOW) { _currentState |= SC_BTN_B; }
      }
    }
    else // Select pin is LOW
    {
      // Check if a controller is connected
      _connected = (bitRead(_inputReg1, DB9_PIN3_BIT) == LOW && bitRead(_inputReg1, DB9_PIN4_BIT) == LOW);
      
      // Check for six button mode
      sixButtonMode = (bitRead(_inputReg3, DB9_PIN1_BIT) == LOW && bitRead(_inputReg4, DB9_PIN2_BIT) == LOW);
      
      // Read input pins for A and Start 
      if(_connected)
      {
        if(!sixButtonMode)
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

word SegaController32U4::getFinalState() {
#ifdef DEBUG
  if ((_previousState == SC_BTN_HOME) && (_currentState == (SC_BTN_HOME | SC_BTN_C))) {
    Serial.print("Read value from EEPROM:");
    Serial.println(isMisterMode());
  }
#endif // DEBUG

  // We carefully check for a change in state (edge trigger) to only toggle
  // MiSTer mode once, even if buttons get pressed a long time.
  if ((_previousState == SC_BTN_HOME) && (_currentState == (SC_BTN_HOME | SC_BTN_Z))) {
    toggleMisterMode();
#ifdef DEBUG
    Serial.print("Wrote value to EEPROM:");
    Serial.println(isMisterMode());
#endif // DEBUG
  }

  if (isMisterMode()) {
    doSwapbuttons(&_currentState, SC_BTN_A, SC_BTN_B);
    doSwapbuttons(&_currentState, SC_BTN_X, SC_BTN_Y);
  }

  _previousState = _currentState;

  word return_value = _currentState;

  // In Mister mode, instead of sending the custom home button, send MODE +
  // DOWN.
  if (isMisterMode() && (return_value & SC_BTN_HOME))
  {
    return_value |= (SC_BTN_DOWN | SC_BTN_MODE);
    return_value &= ~SC_BTN_HOME;
  }
  return return_value;
}
