/*
 * SegaController32U4.h — Genesis/Mega Drive controller (DB9) for 4dapter
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

#ifndef SegaController32U4_h
#define SegaController32U4_h

#define DDR_SELECT DDRE
#define PORT_SELECT PORTE
#define MASK_SELECT B01000000

/*
 SC_BTN_A = 64,
 SC_BTN_B = 16,
 SC_BTN_C = 32,
 SC_BTN_X = 256,
 SC_BTN_Y = 128,
 SC_BTN_Z = 512,
 */

enum
{
  SC_BTN_UP = 1,
  SC_BTN_DOWN = 2,
  SC_BTN_LEFT = 4,
  SC_BTN_RIGHT = 8,
  SC_BTN_A = 32,
  SC_BTN_B = 16,
  SC_BTN_C = 512,
  SC_BTN_X = 128,
  SC_BTN_Y = 64,
  SC_BTN_Z = 256,
  SC_BTN_MODE = 1024,
  SC_BTN_START = 2048,
  SC_BTN_HOME = 4096,
  SC_BIT_SH_UP = 0,
  SC_BIT_SH_DOWN = 1,
  SC_BIT_SH_LEFT = 2,
  SC_BIT_SH_RIGHT = 3,
  DB9_PIN1_BIT = 6,
  DB9_PIN2_BIT = 7,
  DB9_PIN3_BIT = 5,
  DB9_PIN4_BIT = 4,
  DB9_PIN6_BIT = 3,
  DB9_PIN9_BIT = 1
};

const byte SC_CYCLE_DELAY = 10; // Delay (µs) between setting the select pin and reading the button pins

class SegaController32U4
{
 public:
  SegaController32U4();
  word updateState(void);
  word getFinalState(void);

 private:
  word _currentState;

  boolean _pinSelect;

  byte _ignoreCycles;

  boolean _connected;
  boolean _sixButtonMode;

  byte _inputReg1;
  byte _inputReg2;
  byte _inputReg3;
  byte _inputReg4;
};

#endif
