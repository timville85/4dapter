//
// SegaController32U4.h
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

#ifndef SegaController32U4_h
#define SegaController32U4_h

#define DDR_SELECT   DDRE
#define PORT_SELECT  PORTE
#define MASK_SELECT  B01000000

// Set to 1 to use the alternate Genesis button-bit layout (A/C and X/Y/Z
// swapped). No shipped 4dapter build has ever set this to 1; carried forward
// from the original dormant GEN_MISTER flag for compatibility.
#ifndef SC_GEN_MISTER_LAYOUT
#define SC_GEN_MISTER_LAYOUT 0
#endif

#if (SC_GEN_MISTER_LAYOUT)
enum
{
  SC_BTN_UP    = 1,
  SC_BTN_DOWN  = 2,
  SC_BTN_LEFT  = 4,
  SC_BTN_RIGHT = 8,
  SC_BTN_A     = 32,
  SC_BTN_B     = 16,
  SC_BTN_C     = 512,
  SC_BTN_X     = 128,
  SC_BTN_Y     = 64,
  SC_BTN_Z     = 256,
  SC_BTN_MODE  = 1024,
  SC_BTN_START = 2048,
  SC_BTN_HOME  = 4096,
  SC_BIT_SH_UP    = 0,
  SC_BIT_SH_DOWN  = 1,
  SC_BIT_SH_LEFT  = 2,
  SC_BIT_SH_RIGHT = 3,
  DB9_PIN1_BIT = 6,
  DB9_PIN2_BIT = 7,
  DB9_PIN3_BIT = 5,
  DB9_PIN4_BIT = 4,
  DB9_PIN6_BIT = 3,
  DB9_PIN9_BIT = 1
};
#else
enum
{
  SC_BTN_UP    = 1,
  SC_BTN_DOWN  = 2,
  SC_BTN_LEFT  = 4,
  SC_BTN_RIGHT = 8,
  SC_BTN_A     = 64,
  SC_BTN_B     = 16,
  SC_BTN_C     = 32,
  SC_BTN_X     = 256,
  SC_BTN_Y     = 128,
  SC_BTN_Z     = 512,
  SC_BTN_MODE  = 1024,
  SC_BTN_START = 2048,
  SC_BTN_HOME  = 4096,
  SC_BIT_SH_UP    = 0,
  SC_BIT_SH_DOWN  = 1,
  SC_BIT_SH_LEFT  = 2,
  SC_BIT_SH_RIGHT = 3,
  DB9_PIN1_BIT = 6,
  DB9_PIN2_BIT = 7,
  DB9_PIN3_BIT = 5,
  DB9_PIN4_BIT = 4,
  DB9_PIN6_BIT = 3,
  DB9_PIN9_BIT = 1
};
#endif

const byte SC_CYCLE_DELAY = 10; // Delay (µs) between setting the select pin and reading the button pins

class SegaController32U4
{
  public:
    // Full behavior: HOME+Z (edge-triggered) toggles an A/B + X/Y swap ("MiSTer
    // mode"), persisted in EEPROM at |eeprom_index| across power cycles.
    SegaController32U4(int eeprom_index);

    // No EEPROM-backed MiSTer mode: HOME is reported as a raw button and A/B/X/Y
    // are never swapped. Matches the 4-Player HID build.
    SegaController32U4();

    word updateState(void);
    word getFinalState(void);

    // True while the controller is mid-six-button-detection handshake. Public
    // so callers can build their own combo logic on top (used by the Switch
    // build for its 3-vs-6-button special combos).
    boolean sixButtonMode;

  private:
    // Should A/B and X/Y be swapped? No-op when MiSTer mode is disabled
    // (i.e. constructed with the no-argument constructor).
    void toggleMisterMode(void);
    bool isMisterMode(void);

    int _eeprom_index; // -1 = MiSTer mode disabled (no-argument constructor)

    // This acts as a cache to not routinely read EEPROM.
    bool _misterMode;

    word _currentState;
    word _previousState;

    boolean _pinSelect;

    byte _ignoreCycles;

    boolean _connected;

    byte _inputReg1;
    byte _inputReg2;
    byte _inputReg3;
    byte _inputReg4;
};

#endif
