/*
 * ControllerState.h — shared N64 report-encoding helpers for 4dapter
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

#ifndef ControllerState_h
#define ControllerState_h

#include "Arduino.h"
#include "N64_Controller.h"

// Which bit layout to use when packing an N64_status_packet into a button
// bitmask for a HID gamepad report. N64_LAYOUT_STANDARD matches every 4dapter
// HID build except the default Batocera (HID-Single) build, which ships
// N64_LAYOUT_BATOCERA (and duplicates C-Right onto an extra bit — preserved
// here exactly as originally shipped, not "fixed").
enum N64ReportLayout
{
  N64_LAYOUT_STANDARD = 0,
  N64_LAYOUT_BATOCERA = 1,
};

// Packs an N64 controller's raw status into a HID button bitmask. Bit
// positions are preserved verbatim from the original per-sketch code.
inline uint32_t n64ButtonsFromPacket(const N64_status_packet& n64Data, N64ReportLayout layout)
{
  uint32_t n64Buttons = 0;

  if (layout == N64_LAYOUT_STANDARD)
  {
    n64Buttons |= (n64Data.data2 & 0x20 ? 1UL : 0UL) << 4;  // L
    n64Buttons |= (n64Data.data2 & 0x10 ? 1UL : 0UL) << 5;  // R
    n64Buttons |= (n64Data.data2 & 0x08 ? 1UL : 0UL) << 13; // C-Up
    n64Buttons |= (n64Data.data2 & 0x04 ? 1UL : 0UL) << 3;  // C-Down
    n64Buttons |= (n64Data.data2 & 0x02 ? 1UL : 0UL) << 2;  // C-Left
    n64Buttons |= (n64Data.data2 & 0x01 ? 1UL : 0UL) << 6;  // C-Right

    n64Buttons |= (n64Data.data1 & 0x80 ? 1UL : 0UL) << 1;  // A
    n64Buttons |= (n64Data.data1 & 0x40 ? 1UL : 0UL) << 0;  // B
    n64Buttons |= (n64Data.data1 & 0x20 ? 1UL : 0UL) << 8;  // Z
    n64Buttons |= (n64Data.data1 & 0x10 ? 1UL : 0UL) << 7;  // Start
    n64Buttons |= (n64Data.data1 & 0x08 ? 1UL : 0UL) << 9;  // D-Up
    n64Buttons |= (n64Data.data1 & 0x04 ? 1UL : 0UL) << 10; // D-Down
    n64Buttons |= (n64Data.data1 & 0x02 ? 1UL : 0UL) << 11; // D-Left
    n64Buttons |= (n64Data.data1 & 0x01 ? 1UL : 0UL) << 12; // D-Right

    n64Buttons &= 0x0000FFFFUL;
  }
  else // N64_LAYOUT_BATOCERA
  {
    n64Buttons |= (n64Data.data2 & 0x20 ? 1UL : 0UL) << 6;  // L
    n64Buttons |= (n64Data.data2 & 0x10 ? 1UL : 0UL) << 5;  // R
    n64Buttons |= (n64Data.data2 & 0x08 ? 1UL : 0UL) << 3;  // C-Up
    n64Buttons |= (n64Data.data2 & 0x04 ? 1UL : 0UL) << 4;  // C-Down
    n64Buttons |= (n64Data.data2 & 0x02 ? 1UL : 0UL) << 2;  // C-Left
    n64Buttons |= (n64Data.data2 & 0x01 ? 1UL : 0UL) << 13; // C-Right

    n64Buttons |= (n64Data.data1 & 0x80 ? 1UL : 0UL) << 1;  // A
    n64Buttons |= (n64Data.data1 & 0x40 ? 1UL : 0UL) << 0;  // B
    n64Buttons |= (n64Data.data1 & 0x20 ? 1UL : 0UL) << 14; // Z
    n64Buttons |= (n64Data.data1 & 0x10 ? 1UL : 0UL) << 7;  // Start
    n64Buttons |= (n64Data.data1 & 0x08 ? 1UL : 0UL) << 9;  // D-Up
    n64Buttons |= (n64Data.data1 & 0x04 ? 1UL : 0UL) << 10; // D-Down
    n64Buttons |= (n64Data.data1 & 0x02 ? 1UL : 0UL) << 11; // D-Left
    n64Buttons |= (n64Data.data1 & 0x01 ? 1UL : 0UL) << 12; // D-Right

    n64Buttons &= 0x0000FFFFUL;

    if (n64Data.data2 & 0x01) // C-Right, also duplicated on bit 16 as originally shipped
    {
      n64Buttons |= 0x00010000UL;
    }
  }

  return n64Buttons;
}

// Scales a raw N64 stick axis to the signed 8-bit HID range, applying a
// deadzone. Mirrors the N64MapJoyToMax/N64JoyMax/N64JoyDeadzone logic
// duplicated in every sketch's loop(). |invertForMap| matches the Y-axis
// negation the original code applies only when building the mapped value.
inline int8_t n64StickToHidAxis(int8_t rawValue, bool invertForMap, bool mapToMax, int8_t joyMax, int8_t deadzone)
{
  if (rawValue >= -deadzone && rawValue <= deadzone)
  {
    return 0;
  }

  if (!mapToMax)
  {
    return invertForMap ? (int8_t)(-rawValue) : rawValue;
  }

  int8_t clamped = rawValue;
  if (clamped > joyMax)  clamped = joyMax;
  if (clamped < -joyMax) clamped = -joyMax;

  int16_t mapInput = invertForMap ? (int16_t)(-clamped) : (int16_t)clamped;
  return (int8_t)map(mapInput, -joyMax, joyMax, -128, 127);
}

#endif
