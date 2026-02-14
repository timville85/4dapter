/*
 * Gamepad.h — USB HID gamepad (PluggableUSB) for 4dapter
 *
 * Copyright (C) 2026 4dapter project
 *
 * Original HID/PluggableUSB gamepad approach from NicoHood/HID (2014–2015);
 * later adaptations by Mikael Norrgård (2020). This version modified for 4dapter.
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

#pragma once

#include "HID.h"

/** USB serial string reported to the host (see 4dapter_FW-HID.ino). */
extern const char* usbSerialNumber;

typedef struct
{
  uint32_t buttons : 24;
  int8_t X;
  int8_t Y;
} GamepadReport;


class Gamepad_ : public PluggableUSBModule
{
 private:
  uint8_t reportId;

 protected:
  int getInterface(uint8_t* interfaceCount);
  int getDescriptor(USBSetup& setup);
  uint8_t getShortName(char *name);
  bool setup(USBSetup& setup);

  uint8_t epType[1];
  uint8_t protocol;
  uint8_t idle;

 public:
  GamepadReport _GamepadReport;
  Gamepad_(void);
  void reset(void);
  void send();
  /** True if this instance registered its USB endpoint (avoids sending on unregistered 4th when CDC is enabled). */
  bool isPlugged() const
  {
    return _plugged;
  }

 private:
  bool _plugged;
};

/** Four USB HID gamepads (4 endpoints = 4 players). Defined in the main sketch. */
extern Gamepad_ Gamepad[4];
