/*
 * 4dapter HID Firmware — main sketch (NES, SNES, Genesis, N64 -> USB HID gamepads)
 *
 * Copyright (C) 2026 4dapter project
 *
 * This single sketch replaces what used to be four separate, nearly-identical
 * firmware folders (FW-HID, FW-HID-ALT, FW-HID-Single, FW-4P-HID). Which one
 * you get is selected at build time with the HID_LAYOUT flag below:
 *
 *   HID_LAYOUT_TRIPLE      (default) 3 gamepads: NES+SNES combined, Genesis, N64.
 *   HID_LAYOUT_TRIPLE_ALT  3 gamepads: NES, SNES, Genesis+N64 combined.
 *   HID_LAYOUT_SINGLE      1 gamepad: NES+SNES+Genesis+N64 all combined (Batocera).
 *   HID_LAYOUT_QUAD        4 gamepads, one per port (needs CDC_DISABLED, see below).
 *
 * Build with e.g. `-DHID_LAYOUT=HID_LAYOUT_SINGLE` (Arduino IDE: edit the
 * `#define HID_LAYOUT` line below instead; arduino-cli/CI: see ci/build-all.sh).
 *
 * Original implementations for each controller type are credited in the
 * corresponding shared-library source files (Gamepad, SegaController32U4,
 * N64_Controller, NesSnesShiftReader, under lib/4dapterCore).
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

#define HID_LAYOUT_TRIPLE     0
#define HID_LAYOUT_TRIPLE_ALT 1
#define HID_LAYOUT_SINGLE     2
#define HID_LAYOUT_QUAD       3

#ifndef HID_LAYOUT
#define HID_LAYOUT HID_LAYOUT_TRIPLE
#endif

#include "SegaController32U4.h"
#include "Gamepad.h"
#include "N64_Controller.h"
#include "NesSnesShiftReader.h"
#include "ControllerState.h"

#if (HID_LAYOUT == HID_LAYOUT_QUAD) && !defined(CDC_DISABLED)
#warning "CDC_DISABLED is not defined. Only 3 HID endpoints will be available (4th/N64 will not appear). Copy platform.local.txt to the AVR core folder and clear build cache, or build with arduino-cli using --build-property compiler.{c,cpp}.extra_flags=-DCDC_DISABLED. See README.md."
#endif

// Genesis MiSTer-mode (HOME+Z toggle, EEPROM-persisted A/B+X/Y swap) is part of
// every layout except QUAD, which has never had it (matches the original
// 4-Player HID build exactly).
#ifndef SC_MISTER_EEPROM
#if (HID_LAYOUT == HID_LAYOUT_QUAD)
#define SC_MISTER_EEPROM 0
#else
#define SC_MISTER_EEPROM 1
#endif
#endif

// N64 button bit layout: every layout uses the original fixed layout except
// the default Batocera (SINGLE) build, which ships the alternate layout.
#ifndef N64_REPORT_LAYOUT
#if (HID_LAYOUT == HID_LAYOUT_SINGLE)
#define N64_REPORT_LAYOUT N64_LAYOUT_BATOCERA
#else
#define N64_REPORT_LAYOUT N64_LAYOUT_STANDARD
#endif
#endif

// Number of USB HID gamepad endpoints for this layout.
#if (HID_LAYOUT == HID_LAYOUT_QUAD)
#define GAMEPAD_COUNT 4
#elif (HID_LAYOUT == HID_LAYOUT_SINGLE)
#define GAMEPAD_COUNT 1
#else
#define GAMEPAD_COUNT 3
#endif

// USB serial string (max 20 chars including NULL). Used to identify this device to the host.
const char* usbSerialNumber = "4DAPTER";

// N64 analog stick: map to full HID range (-128..127) when true; raw controller value when false.
#define N64_MAP_JOY_TO_MAX true
#define N64_JOY_MAX        80 // Stick physical range (0-127; OEM typically 75-85)
#define N64_JOY_DEADZONE   3  // Center deadzone to reduce drift

// HID axis values when mapping a digital d-pad to a virtual stick.
#define HID_AXIS_CENTER 0
#define HID_AXIS_POS    0x7F
#define HID_AXIS_NEG    (int8_t)0x80

void sendLatch();
void sendClock();
void sendState();

/** Fills a gamepad report from a button mask and DPAD_* axis bits, no analog fallback. */
static void setReportFromButtonsAndDpad(GamepadReport* report, uint32_t buttons, uint32_t dpadAxes)
{
  report->buttons = buttons;
  report->Y = (dpadAxes & DPAD_DOWN) ? HID_AXIS_POS : (dpadAxes & DPAD_UP) ? HID_AXIS_NEG : HID_AXIS_CENTER;
  report->X = (dpadAxes & DPAD_RIGHT) ? HID_AXIS_POS : (dpadAxes & DPAD_LEFT) ? HID_AXIS_NEG : HID_AXIS_CENTER;
}

/**
 * Resolves one HID axis where a digital d-pad direction takes priority over
 * an analog fallback (used when a port with a d-pad and a port with an
 * analog stick are combined onto the same gamepad, e.g. Genesis+N64 or
 * NES+SNES+Genesis+N64). Matches the original firmware's behavior exactly:
 * the fallback value is left untouched whenever neither digital direction
 * is pressed, rather than being reset to 0.
 */
static int8_t combineAxisWithFallback(bool setPositive, bool setNegative, int8_t fallback)
{
  if (setPositive) return HID_AXIS_POS;
  if (setNegative) return HID_AXIS_NEG;
  return fallback;
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

// One USB HID gamepad per GAMEPAD_COUNT above. Declaration order matters: it
// determines USB endpoint order, so don't reorder this across layouts.
Gamepad_ Gamepad[GAMEPAD_COUNT];

#if SC_MISTER_EEPROM
enum EEPROMIndices { GENESIS_EEPROM };
SegaController32U4 genesisController(GENESIS_EEPROM);
#else
SegaController32U4 genesisController;
#endif

NesSnesShiftReader nesSnesReader;
N64Controller n64Controller;

void setup()
{
  n64Controller.N64_init();
  nesSnesReader.init();

  // N64 data pin: input with pull-up
  DDRD &= ~B00010000;
  PORTD |= B00010000;

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
    word genesisState = 0;
    for (uint8_t cycle = 0; cycle < 8; cycle++)
    {
      genesisState = genesisController.updateState();
    }
    genesisState = genesisController.getFinalState();
    uint32_t genesisDpad = genesisState & 0x0F; // SC_BTN_UP/DOWN/LEFT/RIGHT bit values match DPAD_* exactly.

    uint32_t nesButtons, nesAxes, snesButtons, snesAxes;
    nesSnesReader.read(nesButtons, nesAxes, snesButtons, snesAxes);

    n64Controller.getN64Packet();
    N64_status_packet n64Data = n64Controller.N64_status;
    uint32_t n64Buttons = n64ButtonsFromPacket(n64Data, N64_REPORT_LAYOUT);
    int8_t n64StickX = n64StickToHidAxis(n64Data.stick_x, false, N64_MAP_JOY_TO_MAX, N64_JOY_MAX, N64_JOY_DEADZONE);
    int8_t n64StickY = n64StickToHidAxis(n64Data.stick_y, true, N64_MAP_JOY_TO_MAX, N64_JOY_MAX, N64_JOY_DEADZONE);

#if (HID_LAYOUT == HID_LAYOUT_TRIPLE)
    // Gamepad[0] = NES+SNES combined, Gamepad[1] = Genesis, Gamepad[2] = N64.
    setReportFromButtonsAndDpad(&Gamepad[0]._GamepadReport, nesButtons | snesButtons, nesAxes | snesAxes);
    setReportFromButtonsAndDpad(&Gamepad[1]._GamepadReport, genesisState >> 4, genesisDpad);
    Gamepad[2]._GamepadReport.buttons = n64Buttons;
    Gamepad[2]._GamepadReport.X = n64StickX;
    Gamepad[2]._GamepadReport.Y = n64StickY;

#elif (HID_LAYOUT == HID_LAYOUT_TRIPLE_ALT)
    // Gamepad[0] = NES, Gamepad[1] = SNES, Gamepad[2] = Genesis+N64 combined.
    setReportFromButtonsAndDpad(&Gamepad[0]._GamepadReport, nesButtons, nesAxes);
    setReportFromButtonsAndDpad(&Gamepad[1]._GamepadReport, snesButtons, snesAxes);
    Gamepad[2]._GamepadReport.buttons = n64Buttons | (genesisState >> 4);
    // Genesis's digital d-pad overrides the N64 analog stick when pressed;
    // otherwise the N64 stick value shows through (matches original HID-ALT).
    Gamepad[2]._GamepadReport.Y = combineAxisWithFallback(genesisDpad & DPAD_DOWN, genesisDpad & DPAD_UP, n64StickY);
    Gamepad[2]._GamepadReport.X = combineAxisWithFallback(genesisDpad & DPAD_RIGHT, genesisDpad & DPAD_LEFT, n64StickX);

#elif (HID_LAYOUT == HID_LAYOUT_SINGLE)
    // Gamepad[0] = NES+SNES+Genesis+N64, all combined onto one report.
    uint32_t combinedAxes = nesAxes | snesAxes | genesisDpad;
    Gamepad[0]._GamepadReport.buttons = nesButtons | snesButtons | (genesisState >> 4) | n64Buttons;
    Gamepad[0]._GamepadReport.Y = combineAxisWithFallback(combinedAxes & DPAD_DOWN, combinedAxes & DPAD_UP, n64StickY);
    Gamepad[0]._GamepadReport.X = combineAxisWithFallback(combinedAxes & DPAD_RIGHT, combinedAxes & DPAD_LEFT, n64StickX);

#else // HID_LAYOUT_QUAD
    // Gamepad[0]=NES, [1]=SNES, [2]=Genesis, [3]=N64, each fully separate.
    setReportFromButtonsAndDpad(&Gamepad[0]._GamepadReport, nesButtons, nesAxes);
    setReportFromButtonsAndDpad(&Gamepad[1]._GamepadReport, snesButtons, snesAxes);
    setReportFromButtonsAndDpad(&Gamepad[2]._GamepadReport, genesisState >> 4, genesisDpad);
    Gamepad[3]._GamepadReport.buttons = n64Buttons;
    Gamepad[3]._GamepadReport.X = n64StickX;
    Gamepad[3]._GamepadReport.Y = n64StickY;
#endif

    sendState();
  }
}

void sendState()
{
  // isPlugged() guards against sending on an endpoint the host never
  // actually registered (relevant for the 4th/QUAD endpoint; a no-op for
  // every other layout, where all declared endpoints always register).
  for (uint8_t i = 0; i < GAMEPAD_COUNT; i++)
  {
    if (Gamepad[i].isPlugged())
    {
      Gamepad[i].send();
    }
  }
  __builtin_avr_delay_cycles(16000);
}
