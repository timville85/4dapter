/*
 * 4dapter HID Firmware — main sketch (NES, SNES, Genesis, N64 → 4× USB HID gamepads)
 *
 * Copyright (C) 2026 4dapter project
 *
 * Original implementations for each controller type are credited in the
 * corresponding source files (Gamepad, SegaController32U4, N64_Controller).
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

#include "SegaController32U4.h"
#include "Gamepad.h"
#include "N64_Controller.h"

#if !defined(CDC_DISABLED)
#warning "CDC_DISABLED is not defined. Only 3 HID endpoints will be available (4th/N64 will not appear). Copy platform.local.txt to the AVR core folder and clear build cache. See README.md."
#endif

// USB serial string (max 20 chars including NULL). Used to identify this device to the host.
const char* usbSerialNumber = "4DAPTER";

// N64 analog stick: map to full HID range (-128..127) when true; raw controller value when false.
#define N64_MAP_JOY_TO_MAX    true
#define N64_JOY_MAX           80   // Stick physical range (0-127; OEM typically 75-85)
#define N64_JOY_DEADZONE      3    // Center deadzone to reduce drift

N64Controller n64Controller;
N64_status_packet n64StatusPacket;
int8_t n64StickXHid = 0;  // N64 stick X sent to host as HID axis
int8_t n64StickYHid = 0;  // N64 stick Y sent to host as HID axis

// Port indices: NES and SNES share the same shift-register protocol.
#define PORT_NES     0
#define PORT_SNES    1
#define PORT_GENESIS 2

#define INDEX_BUTTONS  0
#define INDEX_AXES     1

// D-pad axis bits (used for NES/SNES/Genesis d-pad -> HID stick).
#define DPAD_UP    0x01
#define DPAD_DOWN  0x02
#define DPAD_LEFT  0x04
#define DPAD_RIGHT 0x08

#define NTT_INDICATOR_BIT  0x00   // SNES NTT (multitap/keypad) indicator
#define SHIFT_BIT_NO_DATA  0x00   // This shift bit is unused / no button

// HID axis values when mapping d-pad to virtual stick
#define HID_AXIS_CENTER  0
#define HID_AXIS_POS     0x7F
#define HID_AXIS_NEG     (int8_t)0x80

void sendLatch();
void sendClock();
void sendState();

/** Set gamepad report from buttons and d-pad axes (UP/DOWN/LEFT/RIGHT bit layout). */
static void setReportFromButtonsAndDpad(GamepadReport* report, uint32_t buttons, uint32_t dpadAxes)
{
  report->buttons = buttons;
  report->Y = (dpadAxes & DPAD_DOWN) ? HID_AXIS_POS : (dpadAxes & DPAD_UP) ? HID_AXIS_NEG : HID_AXIS_CENTER;
  report->X = (dpadAxes & DPAD_RIGHT) ? HID_AXIS_POS : (dpadAxes & DPAD_LEFT) ? HID_AXIS_NEG : HID_AXIS_CENTER;
}

/** Convert N64 stick value to HID axis with optional deadzone and range mapping. */
static int8_t n64StickToHid(int8_t rawStickValue, bool mapToFullRange)
{
  if (rawStickValue >= -N64_JOY_DEADZONE && rawStickValue <= N64_JOY_DEADZONE)
    return 0;
  if (mapToFullRange)
  {
    if (rawStickValue > N64_JOY_MAX)  rawStickValue = N64_JOY_MAX;
    if (rawStickValue < -N64_JOY_MAX) rawStickValue = -N64_JOY_MAX;
    return (int8_t)map(rawStickValue, -N64_JOY_MAX, N64_JOY_MAX, -128, 127);
  }
  return rawStickValue;
}

/*
 * Controller DB9 pinout (view: face-on to the plug)
 *
 *     Pin layout:    5  4  3  2  1
 *                    9  8  7  6
 *
 * Wiring (Triple Controller V1/V2; *** = change from V1 to V2):
 *
 *   Function          V1        V2        Notes
 *   ---------         --------  -------- --------------------
 *   VCC               VCC       VCC
 *   GND               GND       GND
 *   Shared LATCH      PD1 (2)   PD1 (2)
 *   Shared CLOCK      PD0 (3)   PD0 (3)
 *   NES Data1         PF7 (A0)  PF7 (A0)
 *   NES Data D4       N/C       PB5 (9)  ***
 *   NES Data D3       N/C       PB4 (8)  ***
 *   SNES Data1        PF6 (A1)  PF6 (A1)
 *   SNES Data D2      N/C       PD2 (RX) ***
 *   SNES Data D3      N/C       PD3 (TX) ***
 *   DB9-1             PC6 (5)   PC6 (5)
 *   DB9-2             PD7 (6)   PB2 (6)  ***
 *   DB9-3             PF5 (A2)  PF5 (A2)
 *   DB9-4             PF4 (A3)  PF4 (A3)
 *   DB9-5 (VCC)       VCC       PB2 (16) ***
 *   DB9-6             PB3 (14)  PB3 (14)
 *   DB9-7             PE6 (7)   PE6 (7)
 *   DB9-8 (GND)       GND       GND
 *   DB9-9             PB1 (15)  PB1 (15)
 */

/*
 * NES Power Pad button numbering (reference: http://www.neshq.com/hardgen/powerpad.txt)
 *
 *   SIDE B (top view):
 *   +--------------------------------+
 *   |  SIDE B                        |
 *   |   ___ ___ ___ ___               |
 *   |  | 1 | 2 | 3 | 4 |              |
 *   |   --- --- --- ---               |
 *   |   ___ ___ ___ ___               |
 *   |  | 5 | 6 | 7 | 8 |              |
 *   |   --- --- --- ---               |
 *   |   ___ ___ ___ ___               |
 *   |  | 9 |10 |11 |12 |              |
 *   |   --- --- --- ---               |
 *   +--------------------------------+
 *
 *   SIDE A (top view):
 *   +--------------------------------+
 *   |  SIDE A                        |
 *   |   ___ ___                       |
 *   |  | B3 | B2 |  (top row)        |
 *   |   --- ---                       |
 *   |   ___ ___ ___ ___               |
 *   |  | B8 | R7 | R6 | B5 |          |
 *   |   --- --- --- ---               |
 *   |   ___ ___                       |
 *   |  |B11 |B10 |  (bottom row)     |
 *   |   --- ---                       |
 *   +--------------------------------+
 */

// Four USB HID gamepads (4 endpoints = 4 players): [0]=NES, [1]=SNES, [2]=Genesis, [3]=N64.
Gamepad_ Gamepad[4];
SegaController32U4 genesisController;

// NES/SNES: shift-register state [port][buttons=0 or axes=1]. Filled one bit per clock.
uint32_t nesSnesData[2][2] = {{0, 0}, {0, 0}};
// For each of 32 shift bits: 1 = d-pad axis, 0 = button (bits 4-7 = NES/SNES d-pad).
uint32_t shiftBitIsDpadAxis[32] = {
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
uint16_t genesisControllerState = 0;   // Current Genesis/MD controller buttons + d-pad
bool isNttControllerDetected = false;   // SNES NTT (multitap/keypad) present

// NES: mask for each of 8 shift bits -> button or d-pad bit
uint32_t nesShiftBitMask[8] = {
  0x02,       // bit 0: A
  0x01,       // bit 1: B
  0x40,       // bit 2: Start
  0x80,       // bit 3: Select
  DPAD_UP,    // bit 4: D-Up
  DPAD_DOWN,  // bit 5: D-Down
  DPAD_LEFT,  // bit 6: D-Left
  DPAD_RIGHT  // bit 7: D-Right
};

// NES Power Pad, data line D4 (shift bits 0..7 -> pad buttons)
uint32_t powerPadDataLine4Mask[8] = {
  0x08,   // #4
  0x04,   // #3
  0x800,  // #12
  0x80,   // #8
  SHIFT_BIT_NO_DATA, SHIFT_BIT_NO_DATA, SHIFT_BIT_NO_DATA, SHIFT_BIT_NO_DATA
};

// NES Power Pad, data line D3 (shift bits 0..7 -> pad buttons)
uint32_t powerPadDataLine3Mask[8] = {
  0x02,   // #2
  0x01,   // #1
  0x10,   // #5
  0x100,  // #9
  0x20,   // #6
  0x200,  // #10
  0x400,  // #11
  0x40    // #7
};

// SNES: mask for each of 32 shift bits -> button, d-pad, or NTT
uint32_t snesShiftBitMask[32] = {
  0x01,    // B
  0x04,    // Y
  0x40,    // Start
  0x80,    // Select
  DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT,
  0x02,    // A
  0x08,    // X
  0x10,    // L
  0x20,    // R
  SHIFT_BIT_NO_DATA, NTT_INDICATOR_BIT, SHIFT_BIT_NO_DATA, SHIFT_BIT_NO_DATA,
  0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,
  0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000,
  SHIFT_BIT_NO_DATA,
  0x800000   // NTT End Comms
};

void setup()
{
  // Brief delay so power and USB are stable before init.
  delay(50);

  n64Controller.N64_init();

  // N64 data pin: input with pull-up
  DDRD &= ~B00010000;
  PORTD |= B00010000;

  // NES/SNES shared latch and clock (PD1, PD0): output, low
  DDRD |= B00000011;
  PORTD &= ~B00000011;

  // NES/SNES data pins (A0, A1 / PF7, PF6): input with pull-up
  DDRF &= ~B11000000;
  PORTF |= B11000000;

  // NES Power Pad data pins (PB5, PB4): input with pull-up
  DDRB &= ~B00110000;
  PORTB |= B00110000;

  // DB9 pin 5 (VCC): output high
  DDRB |= B00000100;
  PORTB |= B00000100;

  delay(250);
}

void loop()
{
  while (true)
  {
    // Genesis 6-button: 8 read cycles per full state
    for (uint8_t cycle = 0; cycle < 8; cycle++)
      genesisControllerState = genesisController.updateState();
    genesisControllerState = genesisController.getFinalState();

    // Genesis -> Gamepad[2] (d-pad bit layout matches DPAD_*)
    setReportFromButtonsAndDpad(&Gamepad[2]._GamepadReport, genesisControllerState >> 4, genesisControllerState & 0x0F);

    sendLatch();

    nesSnesData[PORT_NES][INDEX_BUTTONS] = 0;
    nesSnesData[PORT_NES][INDEX_AXES] = 0;
    nesSnesData[PORT_SNES][INDEX_BUTTONS] = 0;
    nesSnesData[PORT_SNES][INDEX_AXES] = 0;
    isNttControllerDetected = false;

    for (uint8_t shiftBitIndex = 0; shiftBitIndex < 32; shiftBitIndex++)
    {
      if (!isNttControllerDetected && shiftBitIndex > 13)
        break;

      // NES Power Pad (data lines D4, D3)
      if ((shiftBitIndex < 8) && ((PINB & B00100000) == 0))
        nesSnesData[PORT_NES][INDEX_BUTTONS] |= powerPadDataLine4Mask[shiftBitIndex];
      if ((shiftBitIndex < 8) && ((PINB & B00010000) == 0))
        nesSnesData[PORT_NES][INDEX_BUTTONS] |= powerPadDataLine3Mask[shiftBitIndex];

      // NES controller (data line A0)
      if ((shiftBitIndex < 8) && ((PINF & B10000000) == 0))
      {
        if (shiftBitIsDpadAxis[shiftBitIndex])
          nesSnesData[PORT_NES][INDEX_AXES] |= nesShiftBitMask[shiftBitIndex];
        else
          nesSnesData[PORT_NES][INDEX_BUTTONS] |= nesShiftBitMask[shiftBitIndex];
      }

      // SNES / NTT controller (data line A1)
      if ((PINF & B01000000) == 0)
      {
        if (shiftBitIndex == 13)
          isNttControllerDetected = true;
        if (shiftBitIsDpadAxis[shiftBitIndex])
          nesSnesData[PORT_SNES][INDEX_AXES] |= snesShiftBitMask[shiftBitIndex];
        else
          nesSnesData[PORT_SNES][INDEX_BUTTONS] |= snesShiftBitMask[shiftBitIndex];
      }

      sendClock();
    }

    // NES -> Gamepad[0], SNES -> Gamepad[1]
    setReportFromButtonsAndDpad(&Gamepad[0]._GamepadReport,
        nesSnesData[PORT_NES][INDEX_BUTTONS], nesSnesData[PORT_NES][INDEX_AXES]);
    setReportFromButtonsAndDpad(&Gamepad[1]._GamepadReport,
        nesSnesData[PORT_SNES][INDEX_BUTTONS], nesSnesData[PORT_SNES][INDEX_AXES]);

    // N64 -> Gamepad[3]
    n64Controller.getN64Packet();
    n64StatusPacket = n64Controller.N64_status;

    Gamepad[3]._GamepadReport.X = 0;
    Gamepad[3]._GamepadReport.Y = 0;
    Gamepad[3]._GamepadReport.buttons = 0;

    n64StickXHid = n64StickToHid(n64StatusPacket.stick_x, N64_MAP_JOY_TO_MAX);
    n64StickYHid = n64StickToHid(-n64StatusPacket.stick_y, N64_MAP_JOY_TO_MAX);

    // N64 data2: L, R, C-pad
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data2 & 0x20 ? 1 : 0) << 4;   // L
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data2 & 0x10 ? 1 : 0) << 5;   // R
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data2 & 0x08 ? 1 : 0) << 13;  // C-Up
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data2 & 0x04 ? 1 : 0) << 3;   // C-Down
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data2 & 0x02 ? 1 : 0) << 2;   // C-Left
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data2 & 0x01 ? 1 : 0) << 6;   // C-Right
    // N64 data1: A, B, Z, Start, d-pad
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x80 ? 1 : 0) << 1;   // A
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x40 ? 1 : 0) << 0;   // B
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x20 ? 1 : 0) << 8;   // Z
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x10 ? 1 : 0) << 7;  // Start
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x08 ? 1 : 0) << 9;   // D-Up
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x04 ? 1 : 0) << 10; // D-Down
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x02 ? 1 : 0) << 11;  // D-Left
    Gamepad[3]._GamepadReport.buttons |= (n64StatusPacket.data1 & 0x01 ? 1 : 0) << 12;  // D-Right

    Gamepad[3]._GamepadReport.buttons &= 0x0000FFFF;
    Gamepad[3]._GamepadReport.X = n64StickXHid;
    Gamepad[3]._GamepadReport.Y = n64StickYHid;

    sendState();
  }
}

void sendLatch()
{
  // Latch pulse on shared NES/SNES line (PD1)
  PORTD |= B00000010;
  __builtin_avr_delay_cycles(192);
  PORTD &= ~B00000010;
  __builtin_avr_delay_cycles(72);
}

void sendClock()
{
  // Clock pulse on shared NES/SNES line (PD0)
  PORTD |= B00000001;
  __builtin_avr_delay_cycles(96);
  PORTD &= ~B00000001;
  __builtin_avr_delay_cycles(72);
}

void sendState()
{
  // Push all four gamepad reports to the host (4th only if CDC disabled and endpoint registered).
  Gamepad[0].send();
  Gamepad[1].send();
  Gamepad[2].send();
  if (Gamepad[3].isPlugged())
  {
    Gamepad[3].send();
  }
  __builtin_avr_delay_cycles(16000);
}
