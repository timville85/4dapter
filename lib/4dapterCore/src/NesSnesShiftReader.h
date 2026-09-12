/*
 * NesSnesShiftReader.h — NES/SNES/PowerPad shift-register reader for 4dapter
 *
 * Copyright (C) 2026 4dapter project
 *
 * Extracted from code duplicated across every 4dapter firmware sketch.
 * Protocol reference: http://www.neshq.com/hardgen/powerpad.txt
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

#ifndef NesSnesShiftReader_h
#define NesSnesShiftReader_h

#include "Arduino.h"

// D-pad axis bits used in the |nesAxes|/|snesAxes| outputs of read() below.
#define DPAD_UP    0x01
#define DPAD_DOWN  0x02
#define DPAD_LEFT  0x04
#define DPAD_RIGHT 0x08

// Reads the NES (incl. Power Pad) and SNES (incl. NTT keypad) ports, which
// share the same latch/clock shift-register protocol on this board.
class NesSnesShiftReader
{
  public:
    NesSnesShiftReader();

    // Configures the shared latch/clock/data/Power-Pad pins. Call once from setup().
    void init();

    // Runs one full shift-register read cycle. |nesAxes|/|snesAxes| use the
    // DPAD_* bits above; |nesButtons|/|snesButtons| use each controller's own
    // button bit layout (see the dataMaskNES/dataMaskSNES tables in the .cpp).
    void read(uint32_t& nesButtons, uint32_t& nesAxes, uint32_t& snesButtons, uint32_t& snesAxes);

  private:
    void sendLatch();
    void sendClock();
};

#endif
